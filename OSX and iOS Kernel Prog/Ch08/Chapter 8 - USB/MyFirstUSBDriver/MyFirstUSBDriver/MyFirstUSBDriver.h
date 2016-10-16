#ifndef __MY_FIRST_USB_DRIVER__
#define __MY_FIRST_USB_DRIVER__
#include <IOKit/usb/IOUSBDevice.h>

class com_osxkernel_MyFirstUSBDriver : public IOService
{
    OSDeclareDefaultStructors(com_osxkernel_MyFirstUSBDriver)
    
public:
    virtual bool 	init(OSDictionary *propTable);
    
    virtual IOService* probe(IOService *provider, SInt32 *score );
    
    virtual bool attach(IOService *provider);
    virtual void detach(IOService *provider);
    
    virtual bool start(IOService *provider);
    virtual void stop(IOService *provider);
    
    virtual bool terminate(IOOptionBits options = 0);
};

#endif