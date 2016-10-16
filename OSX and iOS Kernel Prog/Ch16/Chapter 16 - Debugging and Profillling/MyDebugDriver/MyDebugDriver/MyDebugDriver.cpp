#include <IOKit/IOKitDebug.h>
#include <libkern/OSDebug.h>
#include "MyDebugDriver.h"
#include <kern/debug.h>

#define super IOService
OSDefineMetaClassAndStructors(com_osxkernel_MyDebugDriver, IOService);

void com_osxkernel_MyDebugDriver::testFunc3(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4)
{
    if (arg3 == 0)
    {
        com_osxkernel_MyDebugDriver *thisWillNotWork = NULL;
        thisWillNotWork->fVariable1 = arg3;
    }
}

void com_osxkernel_MyDebugDriver::testFunc2(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4)
{
    IOLog("Called testFunc2\n");
    testFunc3(arg1, arg2, arg3, arg4);
}

void com_osxkernel_MyDebugDriver::testFunc1(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4)
{
    IOLog("Called testFunc1\n");
    testFunc2(arg1, arg2, arg3, arg4);
}

bool com_osxkernel_MyDebugDriver::start(IOService * provider)
{
    if (!super::start(provider))
        return false;
    
    IOLog("%s::start\n", getName());
            
    fTimer = IOTimerEventSource::timerEventSource(this, timerFired);
    if (!fTimer)
        return false;
    
    fTimer->setTimeoutMS(20000);
    
    IOWorkLoop* workLoop = getWorkLoop();
    if (workLoop->addEventSource(fTimer) != kIOReturnSuccess)
    {
        fTimer->release();
        fTimer = NULL;
        return false;
    }
    
    return true;
}

void com_osxkernel_MyDebugDriver::timerFired(OSObject* owner, IOTimerEventSource* sender)
{    
    com_osxkernel_MyDebugDriver* me = (com_osxkernel_MyDebugDriver*)owner;
    
    me->fVariable1++;
    me->fVariable2 = me->fVariable1 * 2;
    
    sender->setTimeoutMS(10000);
    IOLog("fVariable1 = %d fVariable2 = %d\n", me->fVariable1, me->fVariable2);
    
    me->testFunc1(0xfeed, 0xbeef, 0x0, 0x1504);
}

bool com_osxkernel_MyDebugDriver::init(OSDictionary* dict)
{
	bool res = super::init(dict);
	IOLog("IOKitTest::init\n");
    fVariable1 = 1;
    fVariable2 = 1;
	return res;
}

void com_osxkernel_MyDebugDriver::stop(IOService * provider)
{
    IOLog("%s::stop\n", getName());
    
    if (fTimer)
    {
        fTimer->cancelTimeout();
        IOWorkLoop *workLoop = getWorkLoop();
        workLoop->removeEventSource(fTimer);
        fTimer->release();
        fTimer = NULL;
    }
    
    super::stop(provider);
}
