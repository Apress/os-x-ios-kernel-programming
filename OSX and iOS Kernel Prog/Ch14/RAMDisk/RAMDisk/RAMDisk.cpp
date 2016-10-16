#include "RAMDisk.h"
#include "RAMDiskStorageDevice.h"
#include <IOKit/IOLib.h>

/********************/
#define SamplePartitionIdentifier "Sample Partition Scheme"

struct SamplePartitionEntry {
	UInt64		blockStart;
	UInt64		blockCount;
};

struct SamplePartitionTable {
	char					partitionIdentifier[24];
	UInt64					partitionCount;
	SamplePartitionEntry	partitionEntries[30];
};
/**********************/


// Define the superclass
#define super IOService

OSDefineMetaClassAndStructors(com_osxkernel_driver_RAMDisk, IOService)

#define kDiskByteSize		(16*1024*1024)

bool com_osxkernel_driver_RAMDisk::start (IOService *provider)
{
	if (super::start(provider) == false)
		return false;
	
	// Allocate storage for the disk
	m_memoryDesc = IOBufferMemoryDescriptor::withCapacity(kDiskByteSize, kIODirectionOutIn);
	if (m_memoryDesc == NULL)
		return false;
	m_buffer = m_memoryDesc->getBytesNoCopy();
	
	// Write our custom Sample Partition Table to the RAM disk (this allows the Sample Partition Scheme driver to match)
	SamplePartitionTable*		partitionTable = (SamplePartitionTable*)m_buffer;
	memset(partitionTable, 0, sizeof(SamplePartitionTable));
	strncpy(partitionTable->partitionIdentifier, SamplePartitionIdentifier, sizeof(partitionTable->partitionIdentifier)-1);
	partitionTable->partitionCount = 2;
	partitionTable->partitionEntries[0].blockStart = (1*1024*1024)/512;		// Partitition 1 starts 1MiB into the disk
	partitionTable->partitionEntries[0].blockCount = (8*1024*1024)/512;		// Partition 1 is 8MiB long
	partitionTable->partitionEntries[1].blockStart = (9*1024*1024)/512;		// Partitition 2 starts 9MiB into the disk
	partitionTable->partitionEntries[1].blockCount = (7*1024*1024)/512;		// Partition 2 is 7MiB long
	
	// Allocate an IOBlockStorageDevice nub
	if (createBlockStorageDevice() == false)
		return false;
	
	return true;
}

void com_osxkernel_driver_RAMDisk::free (void)
{
	if (m_memoryDesc != NULL)
		m_memoryDesc->release();
	
	super::free();
}

IOByteCount com_osxkernel_driver_RAMDisk::performRead (IOMemoryDescriptor* dstDesc, UInt64 byteOffset, UInt64 byteCount)
{
	return dstDesc->writeBytes(0, (void*)((uintptr_t)m_buffer + byteOffset), byteCount);
}

IOByteCount com_osxkernel_driver_RAMDisk::performWrite (IOMemoryDescriptor* srcDesc, UInt64 byteOffset, UInt64 byteCount)
{
	return srcDesc->readBytes(0, (void*)((uintptr_t)m_buffer + byteOffset), byteCount);
}

bool com_osxkernel_driver_RAMDisk::createBlockStorageDevice ()
{
	com_osxkernel_driver_RAMDiskStorageDevice*	nub = NULL;
	bool		result = false;
	
	// Allocate a new IOBlockStorageDevice nub
	nub = new com_osxkernel_driver_RAMDiskStorageDevice;
	if (nub == NULL)
		goto bail;
	
	// Call the custom init method (passing the block size and overall disk size)
	if (nub->init(kDiskByteSize) == false)
		goto bail;
	
	// Attach the IOBlockStorageDevice to the this driver.
	// This call increments the reference count of the nub object,
	// so we can release our reference at function exit.
	if (nub->attach(this) == false)
		goto bail;
	
	// Allow the upper level drivers to match against the IOBlockStorageDevice
	nub->registerService();
	
	result = true;
	
bail:
	// Unconditionally release the nub object
	if (nub != NULL)
		nub->release();
	
	return result;
}
