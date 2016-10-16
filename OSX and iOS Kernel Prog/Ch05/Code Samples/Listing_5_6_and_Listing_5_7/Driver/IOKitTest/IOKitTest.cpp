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
	OSString*	stopMessage;
	
	// Read a possible custom string to print from the driver property table.
	stopMessage = OSDynamicCast(OSString, getProperty("StopMessage"));
	if (stopMessage)
		IOLog("%s\n", stopMessage->getCStringNoCopy());
	
	super::stop(provider);
}

IOReturn com_osxkernel_driver_IOKitTest::setProperties (OSObject* properties)
{
	OSDictionary*	propertyDict;
	
	// The provided properties object should be an OSDictionary object.
	propertyDict = OSDynamicCast(OSDictionary, properties);
	if (propertyDict != NULL)
	{
		OSObject*		theValue;
		OSString*		theString;
		
		// Read the value corresponding to the key "StopMessage" from the dictionary.
		theValue = propertyDict->getObject("StopMessage");
		theString = OSDynamicCast(OSString, theValue);
		if (theString != NULL)
		{
			// Add the value to the driver's property table.
			setProperty("StopMessage", theString);
			return kIOReturnSuccess;
		}
	}
	
	return kIOReturnUnsupported;
}
