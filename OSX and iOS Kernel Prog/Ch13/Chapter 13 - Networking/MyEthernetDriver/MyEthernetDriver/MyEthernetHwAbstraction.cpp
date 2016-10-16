#include <mach/mach_types.h>
#include <sys/systm.h>
#include <sys/kpi_mbuf.h>
#include <sys/kernel_types.h>
#include <net/kpi_interfacefilter.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/ethernet.h>
#include "MyEthernetDriver.h"
#include "MyEthernetHwAbstraction.h"

#define super OSObject

interface_filter_t          gFilterReference;

static errno_t interfaceFilterPacketRecvived(void *cookie, ifnet_t interface, protocol_family_t protocol, mbuf_t *data, char **frame_ptr)
{
    com_osxkernel_MyEthernetHwAbstraction* hwAbtraction = (com_osxkernel_MyEthernetHwAbstraction*)cookie;
    
    if (!hwAbtraction || ifnet_hdrlen(interface) != ETHER_HDR_LEN)
        return 0;
    
    if (!hwAbtraction->handleIncomingPacket(*data, frame_ptr))
        return EPERM;
    
    return 0;
}

static void interfaceFilterDetached(void *cookie, ifnet_t interface) 
{
    com_osxkernel_MyEthernetHwAbstraction* hwAbtraction = (com_osxkernel_MyEthernetHwAbstraction*)cookie;
    if (!hwAbtraction)
        return;
    hwAbtraction->filterDisabled();
}

OSDefineMetaClassAndStructors(com_osxkernel_MyEthernetHwAbstraction, OSObject);

bool    com_osxkernel_MyEthernetHwAbstraction::init(com_osxkernel_MyEthernetDriver* drv)
{
    if (!super::init())
        return false;
    
    fDriver = drv;
    
    fRegisterMap.interruptStatusRegister = 0;
    
    // Arbitarily picked MAC address for our virtual device. For
    // a real device, this would be retrived from the device's registers
    // be:ef:6c:8e:12:11
    fRegisterMap.address.bytes[0] = 0xbe;
    fRegisterMap.address.bytes[1] = 0xef;
    fRegisterMap.address.bytes[2] = 0xfe;
    fRegisterMap.address.bytes[3] = 0xed;
    fRegisterMap.address.bytes[4] = 0x12;
    fRegisterMap.address.bytes[5] = 0x11;
    
    fMacBcastAddress.bytes[0] = 0xff;
    fMacBcastAddress.bytes[1] = 0xff;
    fMacBcastAddress.bytes[2] = 0xff;
    fMacBcastAddress.bytes[3] = 0xff;
    fMacBcastAddress.bytes[4] = 0xff;
    fMacBcastAddress.bytes[5] = 0xff;
        
    interfaceFilter.iff_cookie     = this;
    interfaceFilter.iff_name       = "com.osxkernel.MyEthernetDriver";
    interfaceFilter.iff_event      = NULL;
    interfaceFilter.iff_ioctl      = NULL;
    interfaceFilter.iff_protocol   = 0; // All protocols
    interfaceFilter.iff_output     = NULL;
    interfaceFilter.iff_detached   = interfaceFilterDetached;
    interfaceFilter.iff_input      = interfaceFilterPacketRecvived;
    
    return true;
}

void    com_osxkernel_MyEthernetHwAbstraction::free()
{
    super::free();
}

bool    com_osxkernel_MyEthernetHwAbstraction::enableHardware()
{
    bool success = true;
    
    fRxPacketQueue = IOPacketQueue::withCapacity();
    if (!fRxPacketQueue)
        return false;
    
    if (ifnet_find_by_name("en0", &interface) != KERN_SUCCESS) // change to your own interface
        return false;
    
    ifnet_set_promiscuous(interface, 1);
    
    if (iflt_attach(interface, &interfaceFilter, &gFilterReference) != KERN_SUCCESS)
        success = false;
    
    filterRegistered = true;    
    return success;
}

UInt32    com_osxkernel_MyEthernetHwAbstraction::readRegister32(UInt32 offset)
{
    UInt32 value;
    UInt8* ptr = (UInt8*)&fRegisterMap;
    if (offset > sizeof(MyEthernetDeviceRegisters) - sizeof(UInt32))
        return 0;
    ptr += offset;
    
    value = (UInt32)*ptr;
    // The interrupt register is cleared automatically upon reading.
    if (offset == kMyInterruptStatusRegisterOffset)
        fRegisterMap.interruptStatusRegister = 0;
    
    return value;
}

UInt8    com_osxkernel_MyEthernetHwAbstraction::readRegister8(UInt32 offset)
{
    UInt8 value;
    UInt8* ptr = (UInt8*)&fRegisterMap;
    if (offset > sizeof(MyEthernetDeviceRegisters) - sizeof(UInt32))
        return 0;
    ptr += offset;
    
    value = (UInt8)*ptr;
    // The interrupt register is cleared automatically upon reading.
    if (offset == kMyInterruptStatusRegisterOffset)
        fRegisterMap.interruptStatusRegister = 0;    
    
    return value;
}

void    com_osxkernel_MyEthernetHwAbstraction::disableHardware()
{
    
    if (filterRegistered == true)
    {
        iflt_detach(gFilterReference);
        while (filterRegistered);
        
        ifnet_set_promiscuous(interface, 0);
        ifnet_release(interface);

        fRxPacketQueue->flush();
        fRxPacketQueue->release();
        fRxPacketQueue = NULL;
    }
}

void    com_osxkernel_MyEthernetHwAbstraction::filterDisabled()
{
    filterRegistered = false;
}

bool    com_osxkernel_MyEthernetHwAbstraction::handleIncomingPacket(mbuf_t packet, char** framePtr)
{
    bool passPacketToCaller = true;
    bool copyPacket = false;
    
    struct ether_header *hdr = (struct ether_header*)*framePtr;
    if (!hdr)
        return false;
    
    //
    // We only accept packets routed to us if it is addressed to our Mac address,
    // the broadcast or a multicast address.
    //
    if (memcmp(&fMacBcastAddress.bytes, &hdr->ether_dhost, ETHER_ADDR_LEN) == 0)
    {
        copyPacket = true;
    }
    else if (memcmp(&fRegisterMap.address, &hdr->ether_dhost, ETHER_ADDR_LEN) == 0)
    {
        passPacketToCaller = false; // Belongs to our interface.
        copyPacket = true;
    }
    else if (hdr->ether_dhost[0] & 0x01) // multicast
    {
        copyPacket = true;
    }
    
    if (copyPacket)
    {
        mbuf_t newPacket;
        newPacket = fDriver->allocatePacket((UInt32)mbuf_pkthdr_len(packet) + ETHER_HDR_LEN);
                
        if (newPacket)
        {
            unsigned char* data = (unsigned char*)mbuf_data(newPacket);
            bcopy(*framePtr, data, ETHER_HDR_LEN);
            data += ETHER_HDR_LEN;
            mbuf_copydata(packet, 0, mbuf_pkthdr_len(packet),data);
            
            IOLog("input packet is %lu bytes long\n", mbuf_pkthdr_len(packet));
            
            fRxPacketQueue->lockEnqueue(newPacket);
            fRegisterMap.interruptStatusRegister |= kRXInterruptPending;
            // Raise an interrupt to the driver to inform it of the new packet
            fDriver->fInterruptSource->setTimeoutUS(1);
        }
    }
    return passPacketToCaller;
}

IOReturn    com_osxkernel_MyEthernetHwAbstraction::transmitPacketToHardware(mbuf_t packet)
{    
    if (ifnet_output_raw(interface, 0, packet) != KERN_SUCCESS)
        return kIOReturnOutputDropped;
    
    // Raise an interrupt to the driver to inform it the packet was sent.
    fRegisterMap.interruptStatusRegister |= kTXInterruptPending;
    fDriver->fInterruptSource->setTimeoutUS(1);
    
    return kIOReturnSuccess;
}

mbuf_t  com_osxkernel_MyEthernetHwAbstraction::receivePacketFromHardware()
{
    if (!fRxPacketQueue)
        return NULL;
    return fRxPacketQueue->lockDequeue();
}

