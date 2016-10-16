#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>

#include "HelloKernControl.h"

int main(int argc, char * const *argv)
{
    struct ctl_info ctl_info;
    struct sockaddr_ctl sc;
    char str[MAX_STRING_LEN];
    
    int sock = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
	if (sock < 0) 
		return -1;
	
    bzero(&ctl_info, sizeof(struct ctl_info));
    strcpy(ctl_info.ctl_name, "com.osxkernel.HelloKernControl");
    
    if (ioctl(sock, CTLIOCGINFO, &ctl_info) == -1) 
		return -1;
    
	bzero(&sc, sizeof(struct sockaddr_ctl));
	sc.sc_len = sizeof(struct sockaddr_ctl);
	sc.sc_family = AF_SYSTEM;
	sc.ss_sysaddr = SYSPROTO_CONTROL;
	sc.sc_id = ctl_info.ctl_id;
	sc.sc_unit = 0;
    
	if (connect(sock, (struct sockaddr *)&sc, sizeof(struct sockaddr_ctl))) 
        return -1;
    
    /* get an existing string from the kernel */
    unsigned int size = MAX_STRING_LEN;
    if (getsockopt(sock, SYSPROTO_CONTROL, HELLO_CONTROL_GET_STRING, &str, &size) == -1)
        return -1;
    
    printf("kernel string is: %s\n", str);
    
    /* set a new string */
    strcpy(str, "Hello Kernel, here's your new string, enjoy!");
    if (setsockopt(sock, SYSPROTO_CONTROL, HELLO_CONTROL_SET_STRING, str, (socklen_t)strlen(str)) == -1)
        return -1;

    close(sock);
	
	return 0;
}
