//
//  main.c
//  DriverIterator
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

int main (int argc, const char * argv[])
{
	CFDictionaryRef		matchingDict = NULL;
	io_iterator_t		iter = 0;
	io_service_t		service = 0;
	kern_return_t		kr;
	
	// Create a matching dictionary that will find any USB device
	matchingDict = IOServiceMatching("IOUSBDevice");
	
	// Create an iterator for all IO Registry objects that match the dictionary
	kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter);
	if (kr != KERN_SUCCESS)
		return -1;
	
	// Iterate over all matching objects
	while ((service = IOIteratorNext(iter)) != 0)
	{
		CFStringRef	className;
		io_name_t	name;
		
		// List all IOUSBDevice objects, ignoring objects that subclass IOUSBDevice
		className = IOObjectCopyClass(service);
		if (CFEqual(className, CFSTR("IOUSBDevice")) == true)
		{
			IORegistryEntryGetName(service, name);
			printf("Found device with name: %s\n", name);
		}
		CFRelease(className);
		IOObjectRelease(service);
	}
	
	// Release the iterator
	IOObjectRelease(iter);
	
	return 0;
}
