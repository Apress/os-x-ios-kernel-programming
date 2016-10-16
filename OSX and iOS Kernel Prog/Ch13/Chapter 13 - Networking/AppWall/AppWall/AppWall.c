#include <mach/mach_types.h>
#include <mach/vm_types.h>
#include <mach/kmod.h>
#include <sys/socket.h>
#include <sys/kpi_socket.h>
#include <sys/kpi_mbuf.h>
#include <sys/kpi_socket.h>
#include <sys/kpi_socketfilter.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include <sys/proc.h>

#include <netinet/in.h>
#include <kern/task.h>
#include <kern/locks.h>
#include <kern/assert.h>
#include <kern/debug.h>

#include <libkern/OSMalloc.h>

#include <sys/kern_control.h>

#include "AppWall.h"

static lck_mtx_t        *g_mutex = NULL;
static lck_grp_t        *g_mutex_group = NULL;

static boolean_t        g_filter_registered = FALSE;
static boolean_t        g_filter_unregister_started = FALSE;
static OSMallocTag		g_osm_tag;

TAILQ_HEAD(appwall_entry_list, appwall_entry);
static struct appwall_entry_list g_block_list;

static void log_ip_and_port_addr(struct sockaddr_in* addr)
{
    unsigned char addstr[256];
    inet_ntop(AF_INET, &addr->sin_addr, (char*)addstr, sizeof(addstr));
    printf("%s:%d\n", addstr, ntohs(addr->sin_port));    
}

static struct appwall_entry *
find_entry_by_name(const char *name)
{
	struct appwall_entry *entry, *next_entry;
    	    
    for (entry = TAILQ_FIRST(&g_block_list); entry; entry = next_entry) 
    {
        next_entry = TAILQ_NEXT(entry, link);
        if (strcmp(name, entry->desc.name) == 0)
            return entry;
    }
    return NULL;
}

static	errno_t appwall_attach(void **cookie, socket_t so)
{
    errno_t                 result = 0;
    struct appwall_entry*   entry;
    char                    name[PATH_MAX];
    
    *cookie = NULL;
    
    proc_selfname(name, PATH_MAX);
    
    lck_mtx_lock(g_mutex);
    
    entry = find_entry_by_name(name);
    if (entry)
    {
        entry->users++;
        *cookie = (void*)entry;
        printf("AppWall: attaching to process: %s\n", name);
    }
    else
        result = ENOPOLICY; // don't attach to this socket.
    
    lck_mtx_unlock(g_mutex);
   
	return result;
}

static void	
appwall_detach(void *cookie, socket_t so)
{
    struct appwall_entry*       entry;

    
    if (cookie)
    {
        entry = (struct appwall_entry*)cookie;
        
        lck_mtx_lock(g_mutex);

        entry->users--;
        if (entry->users == 0)
        {
            printf("report for: %s\n", entry->desc.name);
            printf("===================================\n");
            
            if (entry->desc.do_block)
            {
                printf("inbound_blocked: %d\n", entry->desc.inbound_blocked);
                printf("outbound_blocked: %d\n", entry->desc.outbound_blocked);                  
            }
            else
            {
                printf("bytes_in: %lu\n", entry->desc.bytes_in);
                printf("bytes_out: %lu\n", entry->desc.bytes_out);
                printf("entry->desc.packets_in: %lu\n", entry->desc.packets_in);
                printf("entry->desc.packets_out: %lu\n",entry->desc.packets_out);
            }
         
            cookie = NULL;
        }        
        lck_mtx_unlock(g_mutex);
    }
	return;
}

static	errno_t	
appwall_data_in(void *cookie, socket_t so, const struct sockaddr *from,
                  mbuf_t *data, mbuf_t *control, sflt_data_flag_t flags)
{
	struct appwall_entry*       entry;
	errno_t						result = 0;
    
    
    entry = (struct appwall_entry*)cookie;
    if (!entry)
        goto bail;
    
    lck_mtx_lock(g_mutex);    
    entry->desc.bytes_in += mbuf_pkthdr_len(*data);
    entry->desc.packets_in++;
    
    if (entry->desc.do_block)
        result = EPERM;
    
    lck_mtx_unlock(g_mutex);
      
bail:    
    
	return result;
}

static	errno_t	
appwall_data_out(void *cookie, socket_t so, const struct sockaddr *to, mbuf_t *data,
                   mbuf_t *control, sflt_data_flag_t flags)
{
	struct appwall_entry*       entry;
	errno_t						result = 0;
            
    entry = (struct appwall_entry*)cookie;
    if (!entry)
        goto bail;

    lck_mtx_lock(g_mutex);
    entry->desc.bytes_out += mbuf_pkthdr_len(*data);
    entry->desc.packets_out++;
    
    if (entry->desc.do_block)
        result = EPERM;
    lck_mtx_unlock(g_mutex);
bail:
	return result;
}

static	void
appwall_unregistered(sflt_handle handle)
{
    g_filter_registered = FALSE;
}

static	errno_t	
appwall_connect_in(void *cookie, socket_t so, const struct sockaddr *from)
{
	struct appwall_entry*       entry;
	errno_t						result = 0;
            
    entry = (struct appwall_entry*)cookie;
    if (!entry)
        goto bail;
    
    lck_mtx_lock(g_mutex);
    
    if (entry->desc.do_block)
    {
        printf("blocked incoming connection to: %s", entry->desc.name); 
        if (from)
        {
            printf(" from: ");
            log_ip_and_port_addr((struct sockaddr_in*)from);
        }        
        entry->desc.inbound_blocked++;
        result = EPERM;
    }
    lck_mtx_unlock(g_mutex);
bail:
    
	return result;
}

static	errno_t	
appwall_connect_out(void *cookie, socket_t so, const struct sockaddr *to)
{
	struct appwall_entry*       entry;
	errno_t						result = 0;

    entry = (struct appwall_entry*)cookie;
    if (!entry)
        goto bail;
    
    lck_mtx_lock(g_mutex);
    if (entry->desc.do_block)
    {
        printf("blocked outgoing connection from: %s", entry->desc.name); 
        if (to)
        {
            printf(" to: ");
            log_ip_and_port_addr((struct sockaddr_in*)to);
        }
        entry->desc.outbound_blocked++;
        result = EPERM;
    }
    lck_mtx_unlock(g_mutex);
bail:    
    
	return result;
}

#define APPWALL_FLT_TCP_HANDLE       'apw0'      // code should registered with Apple for distributed software

static struct sflt_filter socket_tcp_filter = {
	APPWALL_FLT_TCP_HANDLE,
	SFLT_GLOBAL,
	BUNDLE_ID,
	appwall_unregistered,
	appwall_attach,			
	appwall_detach,
	NULL,
	NULL,
	NULL,
	appwall_data_in,
	appwall_data_out,
	appwall_connect_in,
	appwall_connect_out,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static int add_entry(const char *app_name, int block)
{
    int ret = 0;
    struct appwall_entry* entry = NULL;

    lck_mtx_lock(g_mutex);
            
    entry = find_entry_by_name(app_name);
    if (entry)
    {
        entry->desc.do_block = block;
    }
    else
    {
        /* not found create new entry */
        entry = OSMalloc(sizeof(struct appwall_entry), g_osm_tag);
        if (!entry)
        {
            ret = ENOMEM;
            lck_mtx_unlock(g_mutex);
            return ret;
        }
                
        strncpy(entry->desc.name, app_name, PATH_MAX);
        entry->desc.bytes_in = 0;
        entry->desc.bytes_out = 0;
        entry->desc.inbound_blocked = 0;
        entry->desc.outbound_blocked = 0;
        entry->desc.packets_in = 0;
        entry->desc.packets_out = 0;
        entry->desc.do_block = block;
        entry->users = 0;
                
        TAILQ_INSERT_TAIL(&g_block_list, entry, link);
    }
    
    lck_mtx_unlock(g_mutex);
	
	return ret;

}

kern_return_t AppWall_start (kmod_info_t * ki, void * d) {
    
    int ret;
    
    TAILQ_INIT(&g_block_list);
    
    g_osm_tag = OSMalloc_Tagalloc(BUNDLE_ID, OSMT_DEFAULT);
    if (!g_osm_tag)
        goto bail;
        
    /* allocate mutex group and a mutex to protect global data. */
    g_mutex_group = lck_grp_alloc_init(BUNDLE_ID, LCK_GRP_ATTR_NULL);
    if (!g_mutex_group)
        goto bail;
        
    g_mutex = lck_mtx_alloc_init(g_mutex_group, LCK_ATTR_NULL);
    if (!g_mutex)
        goto bail;
       
    ret = sflt_register(&socket_tcp_filter, PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ret != KERN_SUCCESS)
        goto bail;
    
    add_entry("ssh", 1);    // block the ssh application.
    add_entry("nc", 0);     // log data from the nc application.
   
    g_filter_registered = TRUE;
    
    return KERN_SUCCESS;
bail:
    
    if (g_mutex)
    {
        lck_mtx_free(g_mutex, g_mutex_group);
        g_mutex = NULL;
    }
    if (g_mutex_group)
    {
        lck_grp_free(g_mutex_group);
        g_mutex_group = NULL;
    }
    if (g_osm_tag)
    {
        OSMalloc_Tagfree(g_osm_tag);
        g_osm_tag = NULL;
    }
    
    return KERN_FAILURE;
}

kern_return_t AppWall_stop (kmod_info_t * ki, void * d) 
{    
    int ret = KERN_SUCCESS;
    struct appwall_entry *entry, *next_entry;
    
    if (g_filter_registered == TRUE && !g_filter_unregister_started)
    {        
        sflt_unregister(APPWALL_FLT_TCP_HANDLE);
        g_filter_unregister_started = TRUE;
    }
    // If filter still not unregistered defer stop.
    if (g_filter_registered)
    {
        return EBUSY;
    }
    
    lck_mtx_lock(g_mutex);
    
    /* cleanup */
    for (entry = TAILQ_FIRST(&g_block_list); entry; entry = next_entry) 
    {
        next_entry = TAILQ_NEXT(entry, link);
        TAILQ_REMOVE(&g_block_list, entry, link);
        OSFree(entry, sizeof(struct appwall_entry), g_osm_tag);
    }
    
    lck_mtx_unlock(g_mutex);
    
    lck_mtx_free(g_mutex, g_mutex_group);
    lck_grp_free(g_mutex_group);
    g_mutex = NULL;
    g_mutex_group = NULL;
    
    OSMalloc_Tagfree(g_osm_tag);
    g_osm_tag = NULL;
    
    return ret;
}
