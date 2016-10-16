#include "MyFirstPCIDriver.h"

#define super IOService
OSDefineMetaClassAndStructors(com_osxkernel_MyFirstPCIDriver, IOService);

bool com_osxkernel_MyFirstPCIDriver::start(IOService * provider)
{    
    IOLog("%s::start\n", getName());
    
    if(!super::start(provider))
        return false ;
    
    fPCIDevice = OSDynamicCast(IOPCIDevice, provider);
    if (!fPCIDevice)
        return false;
        
    fPCIDevice->setMemoryEnable(true);
  
    for (UInt32 i = 0; i < fPCIDevice->getDeviceMemoryCount(); i++) 
    {
        IODeviceMemory* memoryDesc = fPCIDevice->getDeviceMemoryWithIndex(i);
        if (!memoryDesc)
            continue;
#ifdef __LP64__
        IOLog("region%u: length=%llu bytes\n", i, memoryDesc->getLength());
#else
        IOLog("region%lu: length=%lu bytes\n", i, memoryDesc->getLength());
#endif
    }

    // Create a 1MB buffer for DMA.
    fDMABuffer = IOBufferMemoryDescriptor::withOptions(kIODirectionOut, 1024 * 1024);
    if (!fDMABuffer)
    {
        return false;
    }
    
    if (prepareDMATransfer() != kIOReturnSuccess)
        IOLog("Failed to prepare DMA transfer\n");
    
    if (fDMABuffer != NULL)
    {
        fDMABuffer->release();
        fDMABuffer = NULL;
    }
    return true;
}

void com_osxkernel_MyFirstPCIDriver::stop(IOService * provider)
{
    IOLog("%s::stop\n", getName());
    
    super::stop(provider);
}

UInt32 com_osxkernel_MyFirstPCIDriver::readRegister32(UInt32 offset)
{
    UInt32 res = 0xffffffff;
    if (!fDeviceRemoved)
    {
        res = OSReadLittleInt32(fBar0Address, offset);
        if (res == 0xffffffff)
            fDeviceRemoved = true;
    }
    return res;
}

IOReturn com_osxkernel_MyFirstPCIDriver::prepareDMATransfer()
{    
    IODMACommand *      dmaCommand;
    IOReturn            ret = kIOReturnSuccess;
    
/*
    // The BAD way.
    IOBufferMemoryDescriptor* fDMABuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, kIODirectionOut, 1024 * 1024, 4096);
    IOByteCount offset = 0;
    while (offset < fDMABuffer->getLength())
    {   
        IOByteCount segmentLength = 0;

#ifdef __LP64__
        addr64_t address = fDMABuffer->getPhysicalSegment(offset, &segmentLength);	
        // In a real driver, we would store the address and length in a S/G list. We just log it here.
        IOLog("Physical segment: address 0x%llx segmentLength: %llu\n", address, segmentLength);
#else
        addr64_t address = fDMABuffer->getPhysicalSegment(offset, &segmentLength, kIOMemoryMapperNone);
        IOLog("Physical segment: address 0x%llx segmentLength: %lu\n", address, segmentLength);
#endif
        offset += segmentLength;
    }
*/    
    
    dmaCommand = IODMACommand::withSpecification(kIODMACommandOutputHost64, 36, 2048, IODMACommand::kMapped, 0, 1);
    if (!dmaCommand)
    {
        IOLog("Failed to allocate IODMACommand\n");
        return kIOReturnNoMemory;
    }
    
    // Will also prepare the memory descriptor.
    ret = dmaCommand->setMemoryDescriptor(fDMABuffer);
    if (ret != kIOReturnSuccess)
        return ret;
            
    UInt64 offset = 0;
    while (offset < fDMABuffer->getLength())
    {
        IODMACommand::Segment64 segment;
        UInt32 numSeg = 1;
            
        ret = dmaCommand->gen64IOVMSegments(&offset, &segment, &numSeg);
        
        IOLog("%s::gen64IOVMSegments() addr 0x%qx, len %llu\n",
              getName(), segment.fIOVMAddr, segment.fLength);
    
        if (ret != kIOReturnSuccess)
            break;
    }
    
    //
    // Setup DMA transfer here for real hardware devices.
    //
    
    if (dmaCommand->clearMemoryDescriptor() != kIOReturnSuccess)
    {
        IOLog("Failed to clear/complete memory descriptor\n");
    }
    
    dmaCommand->release();
    
    return ret;
}

int com_osxkernel_MyFirstPCIDriver::findMSIInterruptTypeIndex()
{
    IOReturn ret;
    int index, source = 0;
    
    for (index = 0; ; index++)
    {
        int interruptType;
        ret = fPCIDevice->getInterruptType(index, &interruptType);
        if (ret != kIOReturnSuccess)
            break;
        
        if (interruptType & kIOInterruptTypePCIMessaged)
        {
            source = index;
            break;
        }
    }
    return source;
}
