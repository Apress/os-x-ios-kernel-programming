#include "IOKitTest.h"
#include <IOKit/IOLib.h>

// Define the superclass
#define super IOService

OSDefineMetaClassAndStructors(com_osxkernel_driver_IOKitTest, IOService)

// Define our power states
enum {
	kOffPowerState,
	kStandbyPowerState,
	kIdlePowerState,
	kOnPowerState,
	//
	kNumPowerStates
};

static IOPMPowerState gPowerStates[kNumPowerStates] = {
	// kOffPowerState
	{kIOPMPowerStateVersion1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	// kStandbyPowerState
	{kIOPMPowerStateVersion1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0},
	// kIdlePowerState
	{kIOPMPowerStateVersion1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0},
	// kOnPowerState
	{kIOPMPowerStateVersion1, kIOPMPowerOn | kIOPMDeviceUsable, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};


bool com_osxkernel_driver_IOKitTest::start (IOService *provider)
{
	if (super::start(provider) == false)
		return false;
	
	// Create a lock for driver/power management synchronization
	m_lock = IOLockAlloc();
	if (m_lock == NULL)
		return false;
	
	// Register driver for power management
	PMinit();
	provider->joinPMtree(this);
	makeUsable();						// Set the private power state to the highest level
	changePowerStateTo(kOffPowerState);	// Set the public power state to the lowest level
	registerPowerDriver(this, gPowerStates, kNumPowerStates);
	
	// Lower the device power level after 5 minutes of activity (expressed in seconds)
	setIdleTimerPeriod(5*60);
	
	return true;
}

void com_osxkernel_driver_IOKitTest::stop (IOService *provider)
{
	PMstop();
	super::stop(provider);
}

void com_osxkernel_driver_IOKitTest::free (void)
{
	if (m_lock)
		IOLockFree(m_lock);
	super::free();
}


IOReturn com_osxkernel_driver_IOKitTest::setPowerState (unsigned long powerStateOrdinal, IOService* device)
{
	// If lowering the power state, update the saved power state before powering down the hardware
	if (powerStateOrdinal < m_devicePowerState)
		m_devicePowerState = powerStateOrdinal;
	
	switch (powerStateOrdinal)
	{
		case kOffPowerState:
			// Wait for outstanding IO to complete before putting device to sleep
			IOLockLock(m_lock);
				while (m_outstandingIO != 0)
				{
					IOLockSleep(m_lock, &m_outstandingIO, THREAD_UNINT);
				}
			IOLockUnlock(m_lock);
			
			// Prepare our hardware for sleep
			// ...
			break;
	}
	
	// If raising the power state, update the saved power state after reinitializing the hardware
	if (powerStateOrdinal > m_devicePowerState)
		m_devicePowerState = powerStateOrdinal;
	
	return kIOPMAckImplied;
}

void	com_osxkernel_driver_IOKitTest::powerChangeDone (unsigned long stateNumber)
{
	// Wake any threads that are blocked on a power state change
	IOLockWakeup(m_lock, &m_devicePowerState, false);
}


// *** Sample Device Operation *** //
IOReturn com_osxkernel_driver_IOKitTest::myReadDataFromDevice ()
{
	// Ensure the device is in the on power state
	IOLockLock(m_lock);
		if (activityTickle(kIOPMSuperclassPolicy1, kOnPowerState) == false)
		{
			// Wait until the device transitions to the on state
			while (m_devicePowerState != kOnPowerState)
			{
				IOLockSleep(m_lock, &m_devicePowerState, THREAD_UNINT);
			}
		}
		
		// Increment the number of outstanding operations
		m_outstandingIO += 1;
	IOLockUnlock(m_lock);
	
	// Perform device read ...
	
	// When the operation is complete, decrement the number of
	// outstanding operations
	IOLockLock(m_lock);
		m_outstandingIO -= 1;
		// Wake any threads that are waiting for a change in the number of outstanding operations
		IOLockWakeup(m_lock, &m_outstandingIO, false);
	IOLockUnlock(m_lock);
	
	return kIOReturnSuccess;
}


IOReturn com_osxkernel_driver_IOKitTest::setProperties (OSObject* properties)
{
	OSDictionary*	propertyDict;
	
	IOLog("Set properties\n");
	// The provided properties object should be an OSDictionary object
	propertyDict = OSDynamicCast(OSDictionary, properties);
	if (propertyDict != NULL)
	{
		if (propertyDict->getObject("Tickle") != NULL)
		{
			myReadDataFromDevice();
			
			return kIOReturnSuccess;
		}
	}
			
	return kIOReturnUnsupported;
}
