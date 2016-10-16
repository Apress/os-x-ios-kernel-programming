
#include "MyEthernetDriver.h"

#include <IOKit/network/IONetworkStats.h>

static struct MediumTable
{
    UInt32	type;
    UInt32	speed;
}
mediumTable[] =
{
    {kIOMediumEthernetNone,											0},
    {kIOMediumEthernetAuto,											0},
    {kIOMediumEthernet10BaseT 	 | kIOMediumOptionHalfDuplex,		10},
    {kIOMediumEthernet10BaseT 	 | kIOMediumOptionFullDuplex,		10},
    {kIOMediumEthernet100BaseTX  | kIOMediumOptionHalfDuplex,	   100},
    {kIOMediumEthernet100BaseTX  | kIOMediumOptionFullDuplex,	   100},
    {kIOMediumEthernet1000BaseT  | kIOMediumOptionFullDuplex,     1000},
};

#define super IOEthernetController

OSDefineMetaClassAndStructors(com_osxkernel_MyEthernetDriver, IOEthernetController);

bool com_osxkernel_MyEthernetDriver::init(OSDictionary *properties)
{
	if (!super::init(properties))
		return false;
    
    return true;
}

bool com_osxkernel_MyEthernetDriver::start(IOService *provider)
{    
    if (!super::start(provider))
        return false;

    fHWAbstraction = new com_osxkernel_MyEthernetHwAbstraction();
    if (!fHWAbstraction)
        return false;
    if (!fHWAbstraction->init(this))
        return false;
    
    if (!createMediumDict())
        return false;
    
    fWorkLoop = getWorkLoop();
    if (!fWorkLoop)
        return false;
    fWorkLoop->retain();

    if (attachInterface((IONetworkInterface**)&fNetworkInterface) == false)
        return false;
    
    fNetworkInterface->registerService();
    
    fInterruptSource = IOTimerEventSource::timerEventSource(this, interruptOccured);
    if (!fInterruptSource)
        return false;
    
    if (fWorkLoop->addEventSource(fInterruptSource) != kIOReturnSuccess)
        return false;
    
    IOLog("%s::start() -> success\n", getName());
    return true;
}

void com_osxkernel_MyEthernetDriver::stop(IOService *provider)
{
    if (fNetworkInterface)
    {
        detachInterface(fNetworkInterface);
    }
    if (fInterruptSource)
    {
        fInterruptSource->cancelTimeout();
        fWorkLoop->removeEventSource(fInterruptSource);
    }
    
    IOLog("stopping\n");    
    
    super::stop(provider);
}

void com_osxkernel_MyEthernetDriver::free()
{
    IOLog("freeing up\n");
    
    if (fInterruptSource)
    {
        fInterruptSource->release();
        fInterruptSource = NULL;
    }

    if (fNetworkInterface)
    {
        fNetworkInterface->release();
        fNetworkInterface = NULL;
    }
    
    if (fHWAbstraction)
    {
        fHWAbstraction->release();
        fHWAbstraction = NULL;
    }
    
    if (fMediumDict)
    {
        fMediumDict->release();
        fMediumDict = NULL;
    }
    
    if (fWorkLoop)
    {
        fWorkLoop->release();
        fWorkLoop = NULL;
    }
    
    super::free();
}

bool com_osxkernel_MyEthernetDriver::createMediumDict()
{
    IONetworkMedium	*medium;
    UInt32		i;
        
    fMediumDict = OSDictionary::withCapacity(sizeof(mediumTable) / sizeof(struct MediumTable));
    if (fMediumDict == 0)
        return false;
    
    for (i = 0; i < sizeof(mediumTable) / sizeof(struct MediumTable); i++)
    {
        medium = IONetworkMedium::medium(mediumTable[i].type, mediumTable[i].speed);
        if (medium)
        {
            IONetworkMedium::addMedium(fMediumDict, medium);
            medium->release();
        }
    }
    
    if (publishMediumDictionary(fMediumDict) != true)
        return false;
    
    medium = IONetworkMedium::getMediumWithType(fMediumDict, kIOMediumEthernetAuto);
    setCurrentMedium(medium);
    return true;
}

bool com_osxkernel_MyEthernetDriver::configureInterface(IONetworkInterface *netif)
{
    IONetworkData	*nd;
        
    if (super::configureInterface(netif) == false)
        return false;
        
    nd = netif->getNetworkData(kIONetworkStatsKey);
    if (!nd || !(fNetworkStats = (IONetworkStats *)nd->getBuffer()))
        return false;
        
    nd = netif->getParameter(kIOEthernetStatsKey);
    if (!nd || !(fEthernetStats = (IOEthernetStats*)nd->getBuffer()))
        return false;
    
    return true;
}

IOReturn com_osxkernel_MyEthernetDriver::enable(IONetworkInterface *netif)
{
    IOMediumType mediumType = kIOMediumEthernet1000BaseT | kIOMediumOptionFullDuplex;
    IONetworkMedium	*medium;

    medium = IONetworkMedium::getMediumWithType(fMediumDict, mediumType);
    
    
    if (!fHWAbstraction->enableHardware())
        return kIOReturnError;
    
    setLinkStatus(kIONetworkLinkActive | kIONetworkLinkValid, medium, 1000 * 1000000);    
    return kIOReturnSuccess;
}

IOReturn com_osxkernel_MyEthernetDriver::disable(IONetworkInterface *netif)
{
    if (fHWAbstraction)
        fHWAbstraction->disableHardware();
    
    setLinkStatus(0, 0);
    return kIOReturnSuccess;
}


IOReturn com_osxkernel_MyEthernetDriver::getHardwareAddress(IOEthernetAddress *addrP)
{
    addrP->bytes[0] = fHWAbstraction->readRegister8(kMyMacAddressRegisterOffset + 0);
    addrP->bytes[1] = fHWAbstraction->readRegister8(kMyMacAddressRegisterOffset + 1);
    addrP->bytes[2] = fHWAbstraction->readRegister8(kMyMacAddressRegisterOffset + 2);
    addrP->bytes[3] = fHWAbstraction->readRegister8(kMyMacAddressRegisterOffset + 3);
    addrP->bytes[4] = fHWAbstraction->readRegister8(kMyMacAddressRegisterOffset + 4);
    addrP->bytes[5] = fHWAbstraction->readRegister8(kMyMacAddressRegisterOffset + 5);

    return kIOReturnSuccess;
}

IOReturn com_osxkernel_MyEthernetDriver::setHardwareAddress(const IOEthernetAddress *addrP)
{
    return kIOReturnUnsupported;
}

UInt32	com_osxkernel_MyEthernetDriver::outputPacket(mbuf_t packet, void *param)
{
    IOReturn result = kIOReturnSuccess;
    if (fHWAbstraction->transmitPacketToHardware(packet) != kIOReturnSuccess)
    {
        result = kIOReturnOutputStall;
    }
    
    return result;
}

#pragma mark Private Methods

void com_osxkernel_MyEthernetDriver::interruptOccured(OSObject* owner, IOTimerEventSource* sender)
{
    mbuf_t packet;

    com_osxkernel_MyEthernetDriver* me = (com_osxkernel_MyEthernetDriver*)owner;
    com_osxkernel_MyEthernetHwAbstraction* hwAbstraction = me->fHWAbstraction;
    if (!me)
        return;
    
    UInt32 interruptStatus = hwAbstraction->readRegister32(kMyInterruptStatusRegisterOffset);
        
    // Recieve interrupt pending, grab packet from hardware. 
    if (interruptStatus & kRXInterruptPending)
    {
        while ((packet = hwAbstraction->receivePacketFromHardware()))
        {
            me->fNetworkInterface->inputPacket(packet);
            me->fNetworkStats->inputPackets++;
        }
        me->fNetworkInterface->flushInputQueue();
    }
    
    if (interruptStatus & kTXInterruptPending)
    {
        // Packet transmitted succesfully.
        me->fNetworkStats->outputPackets++;
    }
}



