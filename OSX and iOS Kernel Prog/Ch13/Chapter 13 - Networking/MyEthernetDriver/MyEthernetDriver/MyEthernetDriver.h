#ifndef MyEthernetDriver_h
#define MyEthernetDriver_h

#include <IOKit/assert.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IOGatedOutputQueue.h>
#include <IOKit/network/IOMbufMemoryCursor.h>
#include <IOKit/network/IONetworkMedium.h>
#include <IOKit/IOUserClient.h>

#include "MyEthernetHwAbstraction.h"

class com_osxkernel_MyEthernetDriver : public IOEthernetController
{
    friend class com_osxkernel_MyEthernetHwAbstraction;
    
	OSDeclareDefaultStructors(com_osxkernel_MyEthernetDriver);
public:
    virtual bool init(OSDictionary *properties);
	virtual bool start(IOService *provider);
	virtual void stop(IOService *provider);
	virtual void free();
    
    virtual bool configureInterface(IONetworkInterface *netif);

    virtual IOReturn enable(IONetworkInterface *netif);
    virtual IOReturn disable(IONetworkInterface *netif); 
    
    virtual IOReturn getHardwareAddress(IOEthernetAddress *addrP);
    
    // Allow our driver's Mac address to be set
    virtual IOReturn setHardwareAddress(const IOEthernetAddress *addrP); 
    
    virtual UInt32 outputPacket(mbuf_t m, void *param);
    
    
    virtual IOReturn setPromiscuousMode(bool active) { return kIOReturnSuccess; }
    virtual IOReturn setMulticastMode(bool active) { return kIOReturnSuccess; }
    
    bool createMediumDict();
    
private:    
    
    static void  interruptOccured(OSObject* owner, IOTimerEventSource* sender);
    
    
    IOTimerEventSource*     fInterruptSource;  // Simulate HW rx interrupt
    IOEthernetInterface*    fNetworkInterface;
    OSDictionary*           fMediumDict;
    IOWorkLoop*             fWorkLoop;
    
    IONetworkStats*         fNetworkStats;
    IOEthernetStats*        fEthernetStats;
    
    com_osxkernel_MyEthernetHwAbstraction* fHWAbstraction; // Low-level hardware acecss.
};

#endif