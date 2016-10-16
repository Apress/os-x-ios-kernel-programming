#include <mach/mach_types.h>
#include <sys/kernel_types.h>
#include <sys/systm.h>
#include <sys/kpi_mbuf.h>
#include <i386/endian.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/kpi_ipfilter.h>

kern_return_t MyIPFilter_start (kmod_info_t * ki, void * d);
kern_return_t MyIPFilter_stop (kmod_info_t * ki, void * d);

enum {
    kMyFiltDirIn,
    kMyFiltDirOut,
    kMyFiltNumDirs
};

struct myfilter_stats
{
    unsigned long udp_packets[kMyFiltNumDirs];
    unsigned long tcp_packets[kMyFiltNumDirs];
    unsigned long icmp_packets[kMyFiltNumDirs];
    unsigned long other_packets[kMyFiltNumDirs];
};

static struct myfilter_stats g_filter_stats;
static ipfilter_t g_filter_ref;
static boolean_t g_filter_registered = FALSE;
static boolean_t g_filter_detached = FALSE;

static void log_ip_packet(mbuf_t *data, int dir)
{
    char src[32], dst[32];
    struct ip *ip = (struct ip*)mbuf_data(*data);
    
    if (ip->ip_v != 4)
        return;
    
    bzero(src, sizeof(src));
    bzero(dst, sizeof(dst));
    inet_ntop(AF_INET, &ip->ip_src, src, sizeof(src));
    inet_ntop(AF_INET, &ip->ip_dst, dst, sizeof(dst));
    
    switch (ip->ip_p) {
        case IPPROTO_TCP:
            printf("TCP: ");
            g_filter_stats.tcp_packets[dir]++;
            break;
        case IPPROTO_UDP:
            printf("UDP: ");
            g_filter_stats.udp_packets[dir]++;
            break;
        case IPPROTO_ICMP:
            printf("ICMP: ");
            g_filter_stats.icmp_packets[dir]++;
        default:
            printf("OTHER: ");
            g_filter_stats.other_packets[dir]++;
            break;
    }
    
    printf("%s -> %s\n", src, dst);
}

static void myipfilter_update_cksum(mbuf_t data)
{
    u_int16_t ip_sum;
    u_int16_t tsum;
    struct tcphdr* tcp;
    struct udphdr* udp;
    
    unsigned char *ptr = (unsigned char*)mbuf_data(data);
    
    struct ip *ip = (struct ip*)ptr;
    if (ip->ip_v != 4)
        return;
    
    ip->ip_sum = 0;
    mbuf_inet_cksum(data, 0, 0, ip->ip_hl << 2, &ip_sum); // ip sum
    
    ip->ip_sum = ip_sum;
    switch (ip->ip_p) {
        case IPPROTO_TCP:
            tcp = (struct tcphdr*)(ptr + (ip->ip_hl << 2));
            tcp->th_sum = 0;
            mbuf_inet_cksum(data, IPPROTO_TCP, ip->ip_hl << 2, ntohs(ip->ip_len) - (ip->ip_hl << 2), &tsum);
            tcp->th_sum = tsum;
            break;
        case IPPROTO_UDP:
            udp = (struct udphdr*)(ptr + (ip->ip_hl << 2));
            udp->uh_sum = 0;
            mbuf_inet_cksum(data, IPPROTO_UDP, ip->ip_hl << 2, ntohs(ip->ip_len) - (ip->ip_hl << 2), &tsum);
            udp->uh_sum = tsum;
            break;
        default:
            break;
    }
    
    mbuf_clear_csum_performed(data); // Needed?
}

 
 //Uncomment to enable re-direction of IP packets to another address.
/*
static errno_t myipfilter_output_redirect(void *cookie, mbuf_t *data, ipf_pktopts_t options)
{
    struct in_addr addr_old;
    struct in_addr addr_new;
    int ret;
    
    struct ip *ip = (struct ip*)mbuf_data(*data);
    if (ip->ip_v != 4)
        return 0;
    
    addr_old.s_addr = htonl(134744072); // 8.8.8.8 
    addr_new.s_addr = htonl(167837964); // 10.1.1.12 (replace with own IP)
    
    // redirect packets to 8.8.8.8 to the IP address 10.1.1.12.
    if (ip->ip_dst.s_addr == addr_old.s_addr)
    {
        ip->ip_dst = addr_new;
        myipfilter_update_cksum(*data);
        ret = ipf_inject_output(*data, g_filter_ref, options);
        return ret == 0 ? EJUSTRETURN : ret;
    }
    return 0;
}

static errno_t myipfilter_input_redirect(void *cookie, mbuf_t *data, int offset, u_int8_t protocol)
{
    struct in_addr addr_old;
    struct in_addr addr_new;
    int ret;
    
    struct ip *ip = (struct ip*)mbuf_data(*data);
    if (ip->ip_v != 4)
        return 0;
    addr_new.s_addr = htonl(134744072); // 8.8.8.8
    addr_old.s_addr = htonl(167837964); // 10.1.1.12 (replace with own IP)
    
    // redirect packets to 8.8.8.8 to the IP address 10.1.1.12.
    if (ip->ip_src.s_addr == addr_old.s_addr)
    {
        ip->ip_src = addr_new;
        // re-inject modified packet.
        myipfilter_update_cksum(*data);
        ret = ipf_inject_input(*data, g_filter_ref);
        printf("redirecting input packet\n");
        return ret == 0 ? EJUSTRETURN : ret;
    }
    return 0;
}
*/


static errno_t myipfilter_output(void *cookie, mbuf_t *data, ipf_pktopts_t options)
{
    if (data)
        log_ip_packet(data, kMyFiltDirOut);
    //return myipfilter_output_redirect(cookie, data, options);
    return 0;
}

static errno_t myipfilter_input(void *cookie, mbuf_t *data, int offset, u_int8_t protocol)
{
    if (data)
        log_ip_packet(data, kMyFiltDirIn);
    //return myipfilter_input_redirect(cookie, data, offset, protocol);
    return 0;
}

static void myipfilter_detach(void *cookie)
{
    /* cookie isn't dynamically allocated, no need to free in this case */
    struct myfilter_stats* stats = (struct myfilter_stats*)cookie;
    printf("UDP_IN %lu UDP OUT: %lu TCP_IN: %lu TCP_OUT: %lu ICMP_IN: %lu ICMP OUT: %lu OTHER_IN: %lu OTHER_OUT: %lu\n",
           stats->udp_packets[kMyFiltDirIn],
           stats->udp_packets[kMyFiltDirOut],
           stats->tcp_packets[kMyFiltDirIn],
           stats->tcp_packets[kMyFiltDirOut],
           stats->icmp_packets[kMyFiltDirIn],
           stats->icmp_packets[kMyFiltDirOut],
           stats->other_packets[kMyFiltDirIn],
           stats->other_packets[kMyFiltDirOut]);
    
    g_filter_detached = TRUE;
}

static struct ipf_filter g_my_ip_filter = { 
    &g_filter_stats,
    "com.osxkernel.MyIPFilter",
    myipfilter_input,
    myipfilter_output,
    myipfilter_detach
};  

kern_return_t MyIPFilter_start (kmod_info_t * ki, void * d) {
    
    int result;
    
    bzero(&g_filter_stats, sizeof(struct myfilter_stats));
    
    result = ipf_addv4(&g_my_ip_filter, &g_filter_ref);
    
    if (result == KERN_SUCCESS)
        g_filter_registered = TRUE;
    
    return result;
}

kern_return_t MyIPFilter_stop (kmod_info_t * ki, void * d) {
    
    if (g_filter_registered)
    {
        ipf_remove(g_filter_ref);
        g_filter_registered = FALSE;

    }
    /* We need to ensure filter is detached before we return */
    if (!g_filter_detached)
        return EAGAIN; // Try unloading again.
    
    return KERN_SUCCESS;
}
