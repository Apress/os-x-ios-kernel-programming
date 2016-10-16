//
//  main.c
//  DriverIterator
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

void DeviceAdded (void* refCon, io_iterator_t iterator)
{
	io_service_t		service = 0;
	
	// Iterate over all matching objects
	while ((service = IOIteratorNext(iterator)) != 0)
	{
		CFStringRef	className;
		
		// List all IOUSBDevice objects, ignoring objects that subclass IOUSBDevice
		className = IOObjectCopyClass(service);
		if (CFEqual(className, CFSTR("IOUSBDevice")) == true)
		{
			CFTypeRef		vendorName;
			
			vendorName = IORegistryEntryCreateCFProperty(service, CFSTR("USB Vendor Name"), kCFAllocatorDefault, 0);
			CFShow(vendorName);
		}
		CFRelease(className);
		IOObjectRelease(service);
	}
}

int main (int argc, const char * argv[])
{
	CFDictionaryRef			matchingDict = NULL;
	io_iterator_t			iter = 0;
	IONotificationPortRef	notificationPort = NULL;
	CFRunLoopSourceRef		runLoopSource;
	kern_return_t			kr;
	
	// Create a matching dictionary that will find any USB device
	matchingDict = IOServiceMatching("IOUSBDevice");
	
	notificationPort = IONotificationPortCreate(kIOMasterPortDefault);
	runLoopSource = IONotificationPortGetRunLoopSource(notificationPort);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	
	kr = IOServiceAddMatchingNotification(notificationPort, kIOFirstMatchNotification, matchingDict, DeviceAdded, NULL, &iter);
	DeviceAdded(NULL, iter);
	
	CFRunLoopRun();
	
	IONotificationPortDestroy(notificationPort);
	
	// Release the iterator
	IOObjectRelease(iter);
	
	return 0;
}

