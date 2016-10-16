//
//  AppWall.h
//  AppWall
//
//  Included by kernel (and a future user-space client).
//

#ifndef AppWall_AppWall_h
#define AppWall_AppWall_h

#define BUNDLE_ID   "com.osxkernel.AppWall"

struct app_descriptor
{
    char name[PATH_MAX];
    unsigned long bytes_in;
    unsigned long bytes_out;
    unsigned long packets_in;
    unsigned long packets_out;
    int           do_block;
    int           outbound_blocked;
    int           inbound_blocked;

};

#if defined (KERNEL)
struct appwall_entry
{
    TAILQ_ENTRY(appwall_entry)   link; 
    struct app_descriptor        desc;
    int                          users;   
};
#endif

#endif
