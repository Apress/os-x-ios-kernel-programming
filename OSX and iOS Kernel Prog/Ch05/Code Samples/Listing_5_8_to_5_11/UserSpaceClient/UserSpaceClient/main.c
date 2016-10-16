//
//  main.c
//  UserSpaceClient
//

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include "UserClientShared.h"


kern_return_t	StartTimer (io_connect_t connection);
kern_return_t	StopTimer (io_connect_t connection);
kern_return_t	GetElapsedTimerTime (io_connect_t connection, uint32_t* timerTime);
kern_return_t	GetElapsedTimerValue (io_connect_t connection, TimerValue* timerValue);
kern_return_t	DelayForMs (io_connect_t connection, uint32_t milliseconds);
kern_return_t	DelayForTime (io_connect_t connection, const TimerValue* timerValue);


kern_return_t	StartTimer (io_connect_t connection)
{
	return IOConnectCallMethod(connection, kTestUserClientStartTimer, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL);
}

kern_return_t	StopTimer (io_connect_t connection)
{
	return IOConnectCallMethod(connection, kTestUserClientStopTimer, NULL, 0, NULL, 0, NULL, NULL, NULL, NULL);
}

kern_return_t	GetElapsedTimerTime (io_connect_t connection, uint32_t* timerTime)
{
	uint64_t		scalarOut[1];
	uint32_t		scalarOutCount;
	kern_return_t	result;
	
	scalarOutCount = 1;		// Initialize to the size of scalarOut array
	result = IOConnectCallScalarMethod(connection, kTestUserClientGetElapsedTimerTime, NULL, 0, scalarOut, &scalarOutCount);
	if (result == kIOReturnSuccess)
		*timerTime = (uint32_t)scalarOut[0];
	
	return result;
}

kern_return_t	GetElapsedTimerValue (io_connect_t connection, TimerValue* timerValue)
{
	size_t	structOutSize;
	
	structOutSize = sizeof(TimerValue);
	return IOConnectCallStructMethod(connection, kTestUserClientGetElapsedTimerValue, NULL, 0, timerValue, &structOutSize);
}

kern_return_t	DelayForMs (io_connect_t connection, uint32_t milliseconds)
{
	uint64_t		scalarIn[1];
	
	scalarIn[0] = milliseconds;
	return IOConnectCallScalarMethod(connection, kTestUserClientDelayForMs, scalarIn, 1, NULL, NULL);
}

kern_return_t	DelayForTime (io_connect_t connection, const TimerValue* timerValue)
{
	return IOConnectCallStructMethod(connection, kTestUserClientDelayForTime, timerValue, sizeof(TimerValue), NULL, 0);
}


IONotificationPortRef	gAsyncNotificationPort = NULL;

IONotificationPortRef	MyDriverGetAsyncCompletionPort ()
{
	// If the port has been allocated, return the existing instance
	if (gAsyncNotificationPort != NULL)
		return gAsyncNotificationPort;
	
	gAsyncNotificationPort = IONotificationPortCreate(kIOMasterPortDefault);
	return gAsyncNotificationPort;
}


kern_return_t	InstallTimer (io_connect_t connection, uint32_t milliseconds, IOAsyncCallback0 timerCallback, void* context)
{
	io_async_ref64_t    asyncRef;
	uint64_t			scalarIn[1];
	
	asyncRef[kIOAsyncCalloutFuncIndex] = (uint64_t)timerCallback;
    asyncRef[kIOAsyncCalloutRefconIndex] = (uint64_t)context;
	
	scalarIn[0] = milliseconds;
	return IOConnectCallAsyncScalarMethod(connection, kTestUserClientInstallTimer, IONotificationPortGetMachPort(gAsyncNotificationPort),
										  asyncRef, kIOAsyncCalloutCount, scalarIn, 1, NULL, NULL);
}


void DelayCallback (void *refcon, IOReturn result)
{
	printf("DelayCallback - refcon %08x and result %08x\n", (uint32_t)refcon, result);
	if (refcon == (void*)0xdeadbeef)
		CFRunLoopStop(CFRunLoopGetMain());
}

int main (int argc, const char * argv[])
{
	CFDictionaryRef		matchingDict = NULL;
	io_iterator_t		iter = 0;
	io_service_t		service = 0;
	kern_return_t		kr;
	
	// Create a matching dictionary that will find any USB device
	matchingDict = IOServiceMatching("com_osxkernel_driver_IOKitTest");
	
	// Create an iterator for all IO Registry objects that match the dictionary
	kr = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDict, &iter);
	if (kr != KERN_SUCCESS)
		return -1;
	
	// Iterate over all matching objects
	while ((service = IOIteratorNext(iter)) != 0)
	{
		task_port_t		owningTask = mach_task_self();
		uint32_t		type = 0;
		io_connect_t	driverConnection;
		
		kr = IOServiceOpen(service, owningTask, type, &driverConnection);
		if (kr == KERN_SUCCESS)
		{
			uint32_t		timerTime;
			TimerValue		timerValue;
			IONotificationPortRef	notificationPort;
			
			notificationPort = MyDriverGetAsyncCompletionPort();
			if (notificationPort)
			{
				CFRunLoopSourceRef		runLoopSource;
				runLoopSource = IONotificationPortGetRunLoopSource(notificationPort);
				CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
			}
			
			kr = StopTimer(driverConnection);
			printf("StopTimer - %08x\n", kr);
			
			kr = StartTimer(driverConnection);
			printf("StartTimer - %08x\n", kr);
			
			kr = GetElapsedTimerTime(driverConnection, &timerTime);
			printf("GetElapsedTimerTime - %08x, time %d\n", kr, timerTime);
			
			kr = DelayForMs(driverConnection, 100);
			printf("DelayForMs - %08x\n", kr);
			
			kr = GetElapsedTimerTime(driverConnection, &timerTime);
			printf("GetElapsedTimerTime - %08x, time %d\n", kr, timerTime);
			
			kr = GetElapsedTimerValue(driverConnection, &timerValue);
			printf("GetElapsedTimerValue - %08x, time %lld / %lld\n", kr, timerValue.time, timerValue.timebase);
			
			timerValue.timebase = 0;
			timerValue.time = 500;
			kr = DelayForTime(driverConnection, &timerValue);
			printf("DelayForTime - %08x\n", kr);
			
			timerValue.timebase = 1000;
			kr = DelayForTime(driverConnection, &timerValue);
			printf("DelayForTime - %08x\n", kr);
			
			kr = GetElapsedTimerTime(driverConnection, &timerTime);
			printf("GetElapsedTimerTime - %08x, time %d\n", kr, timerTime);
			
			kr = StopTimer(driverConnection);
			printf("StopTimer - %08x\n", kr);
			
			kr = InstallTimer(driverConnection, 10000, DelayCallback, (void*)0xdeadbeef);
			printf("InstallTimer - %08x\n", kr);
			kr = InstallTimer(driverConnection, 5000, DelayCallback, (void*)0xdead0005);
			printf("InstallTimer - %08x\n", kr);
			kr = InstallTimer(driverConnection, 2000, DelayCallback, (void*)0xdead0002);
			printf("InstallTimer - %08x\n", kr);
			kr = InstallTimer(driverConnection, 7000, DelayCallback, (void*)0xdead0007);
			printf("InstallTimer - %08x\n", kr);

			CFRunLoopRun();
			
			IONotificationPortDestroy(gAsyncNotificationPort);
			gAsyncNotificationPort = NULL;
			
			IOServiceClose(driverConnection);
		}
		
		IOObjectRelease(service);
	}
	
	// Release the iterator
	IOObjectRelease(iter);
	
	return 0;
}
