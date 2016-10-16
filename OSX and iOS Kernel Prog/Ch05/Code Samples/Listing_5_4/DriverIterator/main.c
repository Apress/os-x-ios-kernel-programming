//
//  main.c
//  DriverIterator
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>

void DeviceNotification (void* refCon, io_service_t service, natural_t messageType, void* messageArgument);


typedef struct {
	io_service_t	service;
	io_object_t		notification;
} MyDriverData;

IONotificationPortRef	gNotificationPort = NULL;


void DeviceAdded (void* refCon, io_iterator_t iterator)
{
	io_service_t		service = 0;
	
	// Iterate over all matching objects.
	while ((service = IOIteratorNext(iterator)) != 0)
	{
		MyDriverData*	myDriverData;
		kern_return_t	kr;
		
		// Allocate a structure to save the driver state.
		myDriverData = (MyDriverData*)malloc(sizeof(MyDriverData));
		myDriverData->service = service;
		
		// Install a callback to receive notifications of driver state changes.
		kr = IOServiceAddInterestNotification(gNotificationPort,
									service,
									kIOGeneralInterest,
									DeviceNotification,		// callback
									myDriverData,			// refCon
									&myDriverData->notification);
	}
}

void DeviceNotification (void* refCon, io_service_t service, natural_t messageType, void* messageArgument)
{
	MyDriverData*	myDriverData = (MyDriverData*)refCon;
	kern_return_t	kr;
	
	// Only handle driver termination notifications.
	if (messageType == kIOMessageServiceIsTerminated)
	{
		// Print the name of the removed device.
		io_name_t	name;
		IORegistryEntryGetName(service, name);
		printf("Device removed: %s\n", name);
		
		// Remove the notification.
		kr = IOObjectRelease(myDriverData->notification);
		// Close the driver connection.
		IOObjectRelease(myDriverData->service);
		// Release our driver state allocation.
		free(myDriverData);
	}
}

int main (int argc, const char * argv[])
{
	CFDictionaryRef			matchingDict = NULL;
	io_iterator_t			iter = 0;
	CFRunLoopSourceRef		runLoopSource;
	kern_return_t			kr;
	
	// Create a matching dictionary that will find any USB device
	matchingDict = IOServiceMatching("IOUSBDevice");
	
	gNotificationPort = IONotificationPortCreate(kIOMasterPortDefault);
	runLoopSource = IONotificationPortGetRunLoopSource(gNotificationPort);
	CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
	
	kr = IOServiceAddMatchingNotification(gNotificationPort, kIOFirstMatchNotification, matchingDict, DeviceAdded, NULL, &iter);
	DeviceAdded(NULL, iter);
	
	CFRunLoopRun();
	
	IONotificationPortDestroy(gNotificationPort);
	
	// Release the iterator
	IOObjectRelease(iter);
	
	return 0;
}

