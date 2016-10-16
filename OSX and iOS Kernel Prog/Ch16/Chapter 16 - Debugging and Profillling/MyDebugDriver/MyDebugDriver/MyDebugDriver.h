#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOTimerEventSource.h>

class com_osxkernel_MyDebugDriver : public IOService
{
    OSDeclareDefaultStructors(com_osxkernel_MyDebugDriver);
public:
    virtual bool init(OSDictionary* dict);
    virtual bool start(IOService* provider);
    virtual void stop(IOService* provider);
    
    void testFunc1(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);
    void testFunc2(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);
    void testFunc3(UInt32 arg1, UInt32 arg2, UInt32 arg3, UInt32 arg4);
    
    static void  timerFired(OSObject* owner, IOTimerEventSource* sender);
private:
    
    IOTimerEventSource*     fTimer;
    int fVariable1;
    int fVariable2;
};