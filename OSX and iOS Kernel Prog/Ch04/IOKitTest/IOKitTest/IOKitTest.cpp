#include "IOKitTest.h"
#include <IOKit/IOLib.h>

// Define the superclass
#define super IOService

OSDefineMetaClassAndStructors(com_osxkernel_driver_IOKitTest, IOService)


bool com_osxkernel_driver_IOKitTest::init (OSDictionary* dict)
{
	bool res = super::init(dict);
	IOLog("IOKitTest::init\n");
	return res;
}

void com_osxkernel_driver_IOKitTest::free (void)
{
	IOLog("IOKitTest::free\n");
	super::free();
}

IOService* com_osxkernel_driver_IOKitTest::probe (IOService* provider, SInt32* score)
{
	IOService *res = super::probe(provider, score);
	IOLog("IOKitTest::probe\n");
	return res;
}

bool com_osxkernel_driver_IOKitTest::start (IOService *provider)
{
	bool res = super::start(provider);
	IOLog("IOKitTest::start\n");
	return res;
}

void com_osxkernel_driver_IOKitTest::stop (IOService *provider)
{
	IOLog("IOKitTest::stop\n");
	super::stop(provider);
}
