#ifndef MyEthernetDriver_MyEthernetHwAbstraction_h
#define MyEthernetDriver_MyEthernetHwAbstraction_h

#include <libkern/c++/OSObject.h>
#include <net/kpi_interfacefilter.h>
#include <IOKit/network/IONetworkController.h>
#include <IOKit/network/IOEthernetController.h>
#include <IOKit/network/IOGatedOutputQueue.h>

enum 
{
    kTXInterruptPending         = 0x1,
    kRXInterruptPending         = 0x2
};

// Pretend "Hardware" Registes
struct MyEthernetDeviceRegisters
{
    IOEthernetAddress   address;
    UInt32              interruptStatusRegister;
} __attribute__((__packed__));

#define kMyMacAddressRegisterOffset       0
#define kMyInterruptStatusRegisterOffset  6


class com_osxkernel_MyEthernetDriver;

class com_osxkernel_MyEthernetHwAbstraction : public OSObject
{
	OSDeclareDefaultStructors(com_osxkernel_MyEthernetHwAbstraction);
public:
    virtual bool    init(com_osxkernel_MyEthernetDriver* drv);
    virtual void    free();
    
    bool            enableHardware();
    void            disableHardware();
    mbuf_t          receivePacketFromHardware();
    IOReturn        transmitPacketToHardware(mbuf_t packet);
    UInt32          readRegister32(UInt32 offset);
    UInt8           readRegister8(UInt32 offset);
    
    bool            handleIncomingPacket(mbuf_t packet, char** framePtr);
    void            filterDisabled();
    
private:
    IOPacketQueue*      fRxPacketQueue;
    IOEthernetAddress   fMacBcastAddress;
    
    bool                        filterRegistered;
    struct iff_filter           interfaceFilter;
    ifnet_t                     interface;
    ifnet_t                     ourInterface;
    
    com_osxkernel_MyEthernetDriver* fDriver;
    
    MyEthernetDeviceRegisters   fRegisterMap;
};

#endif
