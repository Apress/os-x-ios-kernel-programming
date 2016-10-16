//
//  RAMDiskStorageDevice.cpp
//  RAMDisk
//
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "RAMDiskStorageDevice.h"
#include "RAMDisk.h"

// Define the superclass
#define super IOBlockStorageDevice

OSDefineMetaClassAndStructors(com_osxkernel_driver_RAMDiskStorageDevice, IOBlockStorageDevice)

#define kDiskBlockSize		512

bool com_osxkernel_driver_RAMDiskStorageDevice::init(UInt64 diskSize, OSDictionary* properties)
{
	if (super::init(properties) == false)
		return false;
	
	m_blockCount = diskSize / kDiskBlockSize;
	
	return true;
}

bool com_osxkernel_driver_RAMDiskStorageDevice::attach (IOService* provider)
{
	if (super::attach(provider) == false)
		return false;
	
	m_provider = OSDynamicCast(com_osxkernel_driver_RAMDisk, provider);
	if (m_provider == NULL)
		return false;
	
	return true;
}

void com_osxkernel_driver_RAMDiskStorageDevice::detach(IOService* provider)
{
	if (m_provider == provider)
		m_provider = NULL;
	
	super::detach(provider);
}


IOReturn com_osxkernel_driver_RAMDiskStorageDevice::doEjectMedia(void)
{
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::doFormatMedia(UInt64 byteCapacity)
{
	return kIOReturnSuccess;
}

UInt32 com_osxkernel_driver_RAMDiskStorageDevice::doGetFormatCapacities(UInt64* capacities, UInt32 capacitiesMaxCount) const
{
	// Ensure that the array is sufficient to hold all our formats (we require 1 element).
	if ((capacities != NULL) && (capacitiesMaxCount < 1))
		return 0;               // Error, return an array size of 0.
	
	// The caller may provide a NULL array if it wishes to query the number of formats that we support.
	if (capacities != NULL)
		capacities[0] = m_blockCount * kDiskBlockSize;
	return 1;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::doLockUnlockMedia(bool doLock)
{
	return kIOReturnUnsupported;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::doSynchronizeCache(void)
{
	return kIOReturnSuccess;
}

char* com_osxkernel_driver_RAMDiskStorageDevice::getVendorString(void)
{
	return NULL;
	return (char*)"osxkernel.com";
}

char* com_osxkernel_driver_RAMDiskStorageDevice::getProductString(void)
{
	return (char*)"RAM Disk";
}

char* com_osxkernel_driver_RAMDiskStorageDevice::getRevisionString(void)
{
	return (char*)"1.0";
}

char* com_osxkernel_driver_RAMDiskStorageDevice::getAdditionalDeviceInfoString(void)
{
	return NULL;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportBlockSize(UInt64 *blockSize)
{
	*blockSize = kDiskBlockSize;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportEjectability(bool *isEjectable)
{
	*isEjectable = false;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportLockability(bool *isLockable)
{
	*isLockable = false;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportMaxValidBlock(UInt64 *maxBlock)
{
	*maxBlock = m_blockCount-1;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportMediaState(bool *mediaPresent, bool *changedState)
{
	*mediaPresent = true;
	*changedState = false;
	
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportPollRequirements(bool *pollRequired, bool *pollIsExpensive)
{
	*pollRequired = false;
	*pollIsExpensive = false;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportRemovability(bool *isRemovable)
{
	*isRemovable = true;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::reportWriteProtection(bool *isWriteProtected)
{
	*isWriteProtected = false;
	return kIOReturnSuccess;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::getWriteCacheState(bool *enabled)
{
	return kIOReturnUnsupported;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::setWriteCacheState(bool enabled)
{
	return kIOReturnUnsupported;
}

IOReturn com_osxkernel_driver_RAMDiskStorageDevice::doAsyncReadWrite(IOMemoryDescriptor *buffer, UInt64 block, UInt64 nblks, IOStorageAttributes *attributes, IOStorageCompletion *completion)
{
	IODirection		direction;
	IOByteCount		actualByteCount;
	
	// Return errors for incoming I/O if we have been terminated
	if (isInactive() != false )
		return kIOReturnNotAttached;
	
	direction = buffer->getDirection();
	if ((direction != kIODirectionIn) && (direction != kIODirectionOut))
		return kIOReturnBadArgument;
	
	if ((block + nblks) > m_blockCount)
		return kIOReturnBadArgument;
	
	if (direction == kIODirectionIn)
		actualByteCount = m_provider->performRead(buffer, (block*kDiskBlockSize), (nblks*kDiskBlockSize));
	else
		actualByteCount = m_provider->performWrite(buffer, (block*kDiskBlockSize), (nblks*kDiskBlockSize));
	
	(completion->action)(completion->target, completion->parameter, kIOReturnSuccess, actualByteCount);
	
	return kIOReturnSuccess;
}
