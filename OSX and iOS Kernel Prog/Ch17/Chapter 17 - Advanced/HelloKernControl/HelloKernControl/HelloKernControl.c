#include <mach/mach_types.h>
#include <libkern/OSAtomic.h>

#include <sys/systm.h>
#include <sys/proc.h>

#include <sys/proc.h>
#include <sys/kern_control.h>

#include "HelloKernControl.h"

static char             g_string_buf[MAX_STRING_LEN];
static kern_ctl_ref     g_ctl_ref;
static boolean_t        g_filter_registered = FALSE;

static int hello_ctl_connect(kern_ctl_ref ctl_ref, struct sockaddr_ctl *sac, void **unitinfo)
{
    printf("process with pid=%d connected\n", proc_selfpid());
    return 0;
}

static errno_t hello_ctl_disconnect(kern_ctl_ref ctl_ref, u_int32_t unit, void *unitinfo)
{    
    printf("process with pid=%d disconnected\n", proc_selfpid());
	return 0;
}

static int hello_ctl_get(kern_ctl_ref ctl_ref, u_int32_t unit, void *unitinfo, int opt, 
                   void *data, size_t *len)
{
    int ret = 0;
    printf("ctl_get invoked\n");
    
    switch (opt) {
        case HELLO_CONTROL_GET_STRING:
            *len = min(MAX_STRING_LEN, (int)*len);
            strncpy(data, g_string_buf, *len);
            break;
        default:
            ret = ENOTSUP;
            break;
    }
    
    return ret;
}

static int hello_ctl_set(kern_ctl_ref ctl_ref, u_int32_t unit, void *unitinfo, int opt, 
                   void *data, size_t len)
{
    int ret = 0;
    printf("ctl_get invoked\n");

    switch (opt) {
        case HELLO_CONTROL_SET_STRING:
            strncpy(g_string_buf, (char*)data, min(MAX_STRING_LEN, (int)len));
            printf("CONTROL_SET_STRING: new string set to: \"%s\"\n", g_string_buf);
            break;
		default:
			ret = ENOTSUP;
			break;
	}
	return ret;
}

static struct kern_ctl_reg g_kern_ctl_reg = {
	"com.osxkernel.HelloKernControl",              
	0,						
	0,						
	CTL_FLAG_PRIVILEGED,
	0,
	0,		
	hello_ctl_connect,
	hello_ctl_disconnect,
	NULL,
	hello_ctl_set,
	hello_ctl_get
};

kern_return_t HelloKernControl_start (kmod_info_t * ki, void * d) {
	    
    strncpy(g_string_buf, DEFAULT_STRING, strlen(DEFAULT_STRING));
    
    /* register the control */
    int ret = ctl_register(&g_kern_ctl_reg, &g_ctl_ref);
    
    if (ret == KERN_SUCCESS)
    {
        g_filter_registered = TRUE;
        return KERN_SUCCESS;
    }
    return KERN_FAILURE;
}

kern_return_t HelloKernControl_stop (kmod_info_t * ki, void * d) {
    
    if (g_filter_registered)
        ctl_deregister(g_ctl_ref);
    
    return KERN_SUCCESS;
}
