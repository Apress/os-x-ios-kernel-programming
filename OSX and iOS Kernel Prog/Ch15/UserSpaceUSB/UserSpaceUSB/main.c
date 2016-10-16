//
//  main.c
//  UserSpaceUSB
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USBSpec.h>

CFDictionaryRef		MyCreateUSBMatchingDictionary (SInt32 idVendor, SInt32 idProduct);
void				MyFindMatchingDevices (CFDictionaryRef matchingDictionary);
IOUSBDeviceInterface300**	MyStartDriver (io_service_t usbDeviceRef);
IOReturn			PrintDeviceManufacturer (IOUSBDeviceInterface300** usbDevice);
IOReturn			MyConfigureDevice (IOUSBDeviceInterface300** usbDevice);
IOUSBInterfaceInterface300**	MyCreateInterfaceClass (io_service_t usbInterfaceRef);
IOReturn			MyListEndpoints (IOUSBInterfaceInterface300** usbInterface);


int main (int argc, const char * argv[])
{
	CFDictionaryRef 	matchingDictionary = NULL;
	
	// The USB Vendor ID 0x05AC and Product ID 0x8502 corresponds to a particular model of iSight camera.
	// The vendor ID / product ID can be changed to any device that is present on your system.
	matchingDictionary = MyCreateUSBMatchingDictionary(0x05ac, 0x8502);
	if (matchingDictionary != NULL)
		MyFindMatchingDevices(matchingDictionary);
	
    return 0;
}


CFDictionaryRef		MyCreateUSBMatchingDictionary (SInt32 idVendor, SInt32 idProduct)
{
	CFMutableDictionaryRef 	matchingDictionary = NULL;
	CFNumberRef				numberRef;
	
	// Create a matching dictionary for IOUSBDevice
	matchingDictionary = IOServiceMatching(kIOUSBDeviceClassName);
	if (matchingDictionary == NULL)
		goto bail;
	
	// Add the USB Vendor ID to the matching dictionary
	numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idVendor);
	if (numberRef == NULL)
		goto bail;
	CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBVendorID), numberRef);
	CFRelease(numberRef);
	
	// Add the USB Product ID to the matching dictionary
	numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &idProduct);
	if (numberRef == NULL)
		goto bail;
	CFDictionaryAddValue(matchingDictionary, CFSTR(kUSBProductID), numberRef);
	CFRelease(numberRef);
	
	// Success - return the dictionary to the caller
	return matchingDictionary;
	
bail:
	// Failure - release resources and return NULL
	if (matchingDictionary != NULL)
		CFRelease(matchingDictionary);
		
	return NULL;
}

void	MyFindMatchingDevices (CFDictionaryRef matchingDictionary)
{
	io_iterator_t		iterator = 0;
	io_service_t		usbDeviceRef;
	kern_return_t		err;
	
	// Find all kernel objects that match the dictionary
	err = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &iterator);
	if (err == 0)
	{
		// Iterate over all matching kernel objects
		while ((usbDeviceRef = IOIteratorNext(iterator)) != 0)
		{
			// Create a driver for this device instance
			MyStartDriver(usbDeviceRef);
			IOObjectRelease(usbDeviceRef);
		}
		
		IOObjectRelease(iterator);
	}
}

IOUSBDeviceInterface300**	MyStartDriver (io_service_t usbDeviceRef)
{
	SInt32						score;
	IOCFPlugInInterface**		iodev;
	IOUSBDeviceInterface300**	usbDevice = NULL;
	kern_return_t				err;
	
	// Create the IOUSBDeviceInterface object
	err = IOCreatePlugInInterfaceForService(usbDeviceRef, kIOUSBDeviceUserClientTypeID, kIOCFPlugInInterfaceID, &iodev, &score);
	if (err == 0)
	{
		err = (*iodev)->QueryInterface(iodev, CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID300), (LPVOID)&usbDevice);
		IODestroyPlugInInterface(iodev);
	}
	
	if ((*usbDevice)->USBDeviceOpen(usbDevice) == kIOReturnSuccess)
	{
		(*usbDevice)->ResetDevice(usbDevice);
		
		// Read the USB device's manufacturer string
		PrintDeviceManufacturer(usbDevice);
		
		// Set the device's USB configuration
		MyConfigureDevice(usbDevice);
		
		(*usbDevice)->USBDeviceClose(usbDevice);
	}
	
	return usbDevice;
}

IOReturn	PrintDeviceManufacturer (IOUSBDeviceInterface300** usbDevice)
{
	UInt8			stringIndex;
	IOUSBDevRequest	devRequest;
	UInt8			buffer[256];
	CFStringRef		manufString;
	IOReturn		error;
	
	// Get the index in the string table for the manufacturer
	error = (*usbDevice)->USBGetManufacturerStringIndex(usbDevice, &stringIndex);
	if (error != kIOReturnSuccess)
		return error;
	
	// Perform a device request to read the string descriptor
	devRequest.bmRequestType = USBmakebmRequestType(kUSBIn, kUSBStandard, kUSBDevice);
	devRequest.bRequest = kUSBRqGetDescriptor;
	devRequest.wValue = (kUSBStringDesc << 8) | stringIndex;
	devRequest.wIndex = 0x409;		// Language setting - specify US English
	devRequest.wLength = sizeof(buffer);
	devRequest.pData = &buffer[0];
	bzero(&buffer[0], sizeof(buffer));
	//
	error = (*usbDevice)->DeviceRequest(usbDevice, &devRequest);
	if (error != kIOReturnSuccess)
		return error;
	
	// Create a CFString representation of the returned data
	int		strLength;
	strLength = buffer[0] - 2;		// First byte is length (in bytes)
	manufString = CFStringCreateWithBytes(kCFAllocatorDefault, &buffer[2], strLength,
						kCFStringEncodingUTF16LE, false);
	
	CFShow(manufString);
	CFRelease(manufString);
	
	return error;
}


IOReturn	MyConfigureDevice (IOUSBDeviceInterface300** usbDevice)
{
	UInt8							numConfigurations;
	IOUSBConfigurationDescriptorPtr	configDesc;
	IOUSBFindInterfaceRequest		interfaceRequest;
	io_iterator_t					interfaceIterator;
	io_service_t					usbInterfaceRef;
	IOReturn						error;
	
	// Get the count of the device's configurations
	error = (*usbDevice)->GetNumberOfConfigurations(usbDevice, &numConfigurations);
	if ((error != kIOReturnSuccess) || (numConfigurations == 0))
		return error;
	
	// Read the descriptor for the first configuration
	error = (*usbDevice)->GetConfigurationDescriptorPtr(usbDevice, 0, &configDesc);
	if (error != kIOReturnSuccess)
		return error;
	
	// Make the first configuration the active configuration
	error = (*usbDevice)->SetConfiguration(usbDevice, configDesc->bConfigurationValue);
	if (error != kIOReturnSuccess)
		return error;
	
	// Create an iterator over all interfaces in the active configuration
	interfaceRequest.bInterfaceClass = kIOUSBFindInterfaceDontCare;
	interfaceRequest.bInterfaceSubClass = kIOUSBFindInterfaceDontCare;
	interfaceRequest.bInterfaceProtocol = kIOUSBFindInterfaceDontCare;
	interfaceRequest.bAlternateSetting = kIOUSBFindInterfaceDontCare;
	
	error = (*usbDevice)->CreateInterfaceIterator(usbDevice, &interfaceRequest, &interfaceIterator);
	if (error != kIOReturnSuccess)
		return error;
	
	// Iterate over all interfaces
	while ((usbInterfaceRef = IOIteratorNext(interfaceIterator)) != 0)
	{
		IOUSBInterfaceInterface300**	usbInterface;
		
		usbInterface = MyCreateInterfaceClass(usbInterfaceRef);
		IOObjectRelease(usbInterfaceRef);
		
		if ((*usbInterface)->USBInterfaceOpen(usbInterface) == kIOReturnSuccess)
		{
			// Use the interface to iterate the endpoints
			error = MyListEndpoints(usbInterface);
			
			(*usbInterface)->USBInterfaceClose(usbInterface);
		}
	}
	IOObjectRelease(interfaceIterator);
	
	return kIOReturnSuccess;
}

IOUSBInterfaceInterface300**	MyCreateInterfaceClass (io_service_t usbInterfaceRef)
{
	SInt32							score;
	IOCFPlugInInterface**			plugin;
	IOUSBInterfaceInterface300**	usbInterface = NULL;
	kern_return_t					err;
	
	// Create the IOUSBInterfaceInterface object
	err = IOCreatePlugInInterfaceForService(usbInterfaceRef, kIOUSBInterfaceUserClientTypeID, kIOCFPlugInInterfaceID, &plugin, &score);
	if (err == 0)
	{
		err = (*plugin)->QueryInterface(plugin, 
			CFUUIDGetUUIDBytes(kIOUSBInterfaceInterfaceID300),
			(LPVOID)&usbInterface);
		IODestroyPlugInInterface(plugin);
	}
	
	return usbInterface;
}

IOReturn	MyListEndpoints (IOUSBInterfaceInterface300** usbInterface)
{
	UInt8		numEndpoints;
	UInt8		i;
	IOReturn	error;
	
	// Determine the number of endpoints in this interface
	error = (*usbInterface)->GetNumEndpoints(usbInterface, &numEndpoints);
	if (error != kIOReturnSuccess)
		return error;
	
	// Iterate over all endpoints in the interface (skipping endpoint 0, the control endpoint)
	for (i = 1; i <= numEndpoints; i++)
	{
		UInt8		direction, number, transferType;
		UInt16		maxPacketSize;
		UInt8		interval;
		
		error = (*usbInterface)->GetPipeProperties(usbInterface, i, &direction, &number, &transferType, &maxPacketSize, &interval);
		if (error != kIOReturnSuccess)
			return error;
		
		// Print a description of the current endpoint
		if ((transferType == kUSBControl) && (direction == kUSBOut))
			printf("Pipe ref %d: Control OUT\n", i);
		else if ((transferType == kUSBControl) && (direction == kUSBIn))
			printf("Pipe ref %d: Control IN\n", i);
		else if ((transferType == kUSBIsoc) && (direction == kUSBOut))
			printf("Pipe ref %d: Isoc OUT\n", i);
		else if ((transferType == kUSBIsoc) && (direction == kUSBIn))
			printf("Pipe ref %d: Isoc IN\n", i);
		else if ((transferType == kUSBBulk) && (direction == kUSBOut))
			printf("Pipe ref %d: Bulk OUT\n", i);
		else if ((transferType == kUSBBulk) && (direction == kUSBIn))
			printf("Pipe ref %d: Bulk IN\n", i);
		else if ((transferType == kUSBInterrupt) && (direction == kUSBOut))
			printf("Pipe ref %d: Interrupt OUT\n", i);
		else if ((transferType == kUSBInterrupt) && (direction == kUSBIn))
			printf("Pipe ref %d: Interrupt IN\n", i);
	}
	
	return kIOReturnSuccess;
}
