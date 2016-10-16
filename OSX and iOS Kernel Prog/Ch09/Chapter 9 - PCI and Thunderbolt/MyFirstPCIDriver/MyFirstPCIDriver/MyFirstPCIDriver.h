#include <IOKit/IOLib.h>
#include <IOKit/pci/IOPCIDevice.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

class com_osxkernel_MyFirstPCIDriver : public IOService
{
    OSDeclareDefaultStructors(com_osxkernel_MyFirstPCIDriver);
    
private:
    IOPCIDevice*        fPCIDevice;
    
    bool                fDeviceRemoved;
    void*               fBar0Address;
    
    IOBufferMemoryDescriptor* fDMABuffer;
    
    
    int                 findMSIInterruptTypeIndex();
    
    // DMA functions
    IOReturn            prepareDMATransfer();
    
public:
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    
    UInt32 readRegister32(UInt32 offset);
};