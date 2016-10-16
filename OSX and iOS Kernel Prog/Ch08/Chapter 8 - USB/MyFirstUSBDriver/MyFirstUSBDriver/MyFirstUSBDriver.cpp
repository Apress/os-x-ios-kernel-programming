#include <IOKit/IOLib.h>
#include <IOKit/usb/IOUSBInterface.h>
#include "MyFirstUSBDriver.h"

OSDefineMetaClassAndStructors(com_osxkernel_MyFirstUSBDriver, IOService)
#define super IOService

void
logEndpoint(IOUSBPipe* pipe)
{
    IOLog("Endpoint #%d ", pipe->GetEndpointNumber());
    IOLog("--> Type: ");
    switch (pipe->GetType())
    {
        case kUSBControl: IOLog("kUSBControl "); break;
        case kUSBBulk: IOLog("kUSBBulk "); break;
        case kUSBIsoc: IOLog("kUSBIsoc "); break;
        case kUSBInterrupt: IOLog("kUSBInterrupt "); break;
    }
    IOLog("--> Direction: ");
    switch (pipe->GetDirection()) 
    {
        case kUSBOut: IOLog("OUT (kUSBOut) "); break;
        case kUSBIn: IOLog("IN (kUSBIn) "); break;
        case kUSBAnyDirn: IOLog("ANY (Control Pipe) "); break;
    }        
    IOLog("maxPacketSize: %d interval: %d\n", pipe->GetMaxPacketSize(), pipe->GetInterval());    
}

bool 	
com_osxkernel_MyFirstUSBDriver::init(OSDictionary *propTable)
{
    IOLog("com_osxkernel_MyFirstUSBDriver::init(%p)\n", this);
    return super::init(propTable);
}

IOService* 
com_osxkernel_MyFirstUSBDriver::probe(IOService *provider, SInt32 *score)
{
    IOLog("%s(%p)::probe\n", getName(), this);
    return super::probe(provider, score);			
}

bool 
com_osxkernel_MyFirstUSBDriver::attach(IOService *provider)
{
    IOLog("%s(%p)::attach\n", getName(), this);
    return super::attach(provider);
}

void 
com_osxkernel_MyFirstUSBDriver::detach(IOService *provider)
{
    IOLog("%s(%p)::detach\n", getName(), this);
    return super::detach(provider);
}

bool 
com_osxkernel_MyFirstUSBDriver::start(IOService *provider)
{
    IOUSBInterface* interface;
    IOUSBFindEndpointRequest request;
    IOUSBPipe* pipe = NULL;
    
    IOLog("%s(%p)::start\n", getName(), this);

    if (!super::start(provider))
        return false;
    
    interface = OSDynamicCast(IOUSBInterface, provider);
    if (interface == NULL)
    {
        IOLog("%s(%p)::start -> provider not a IOUSBInterface\n", getName(), this);
        return false;
    }
    
    // Mass Storage Devices use two bulk pipes, one for reading and one for writing.
    
    // Find the Bulk In Pipe.
    request.type = kUSBBulk;
    request.direction = kUSBIn;
 	pipe = interface->FindNextPipe(NULL, &request, true);
    if (pipe)
    {
        logEndpoint(pipe);
        pipe->release();
    }
    
    // Find the Bulk Out Pipe.
    request.type = kUSBBulk;
	request.direction = kUSBOut;
	pipe = interface->FindNextPipe(NULL, &request, true);
    if (pipe)
    {
        logEndpoint(pipe);
        pipe->release();
    }
    
    return true;
}

void 
com_osxkernel_MyFirstUSBDriver::stop(IOService *provider)
{
    IOLog("%s(%p)::stop\n", getName(), this);
    super::stop(provider);
}

bool 
com_osxkernel_MyFirstUSBDriver::terminate(IOOptionBits options)
{
    IOLog("%s(%p)::terminate\n", getName(), this);
    return super::terminate(options);
}
