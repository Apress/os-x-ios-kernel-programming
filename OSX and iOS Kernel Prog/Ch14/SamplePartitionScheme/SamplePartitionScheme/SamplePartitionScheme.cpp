#include "SamplePartitionScheme.h"
#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

// Define the superclass
#define super IOPartitionScheme

OSDefineMetaClassAndStructors(com_osxkernel_driver_PartitionScheme, IOPartitionScheme)


IOService* com_osxkernel_driver_PartitionScheme::probe(IOService* provider, SInt32* score)
{
	if (super::probe(provider, score) == 0)
		return 0;
	
	// Scan the IOMedia for a supported partition table
	m_partitions = scan(score);
	
	// If no partition table was found, return NULL
	return m_partitions ? this : NULL;
}

bool com_osxkernel_driver_PartitionScheme::start (IOService *provider)
{
	IOMedia*		partition;
	OSIterator*		partitionIterator;
	
	if (super::start(provider) == false)
		return false;
	
	// Create an iterator for the IOMedia objects that were found and instantiated during probe
	partitionIterator = OSCollectionIterator::withCollection(m_partitions);
	if (partitionIterator == NULL)
		return false;
	
	// Attach and register each IOMedia object (representing found partitions)
	while ((partition = (IOMedia*)partitionIterator->getNextObject()))
	{
		if (partition->attach(this))
		{
			attachMediaObjectToDeviceTree(partition);
			partition->registerService();
		}
	}
	partitionIterator->release();

	return true;
}

void com_osxkernel_driver_PartitionScheme::stop(IOService* provider)
{
	IOMedia*		partition;
	OSIterator*		partitionIterator;
	
	// Detach the media objects we previously attached to the device tree.
	partitionIterator = OSCollectionIterator::withCollection(m_partitions);
	if (partitionIterator)
	{
		while ((partition = (IOMedia*)partitionIterator->getNextObject()))
		{
			detachMediaObjectFromDeviceTree(partition);
		}
		
		partitionIterator->release();
	}
	
	super::stop(provider);
}

IOReturn com_osxkernel_driver_PartitionScheme::requestProbe(IOOptionBits options)
{
	OSSet*	partitions		= 0;
	OSSet*	partitionsNew;
	SInt32	score			= 0;
	
	// Scan the provider media for partitions.
	partitionsNew = scan(&score);
	if (partitionsNew)
	{
		if (lockForArbitration(false))
		{
			partitions = juxtaposeMediaObjects(m_partitions, partitionsNew);
			if (partitions)
			{
				m_partitions->release();
				m_partitions = partitions;
			}
			
			unlockForArbitration();
		}
		
		partitionsNew->release();
	}
	
	return partitions ? kIOReturnSuccess : kIOReturnError;
}


void com_osxkernel_driver_PartitionScheme::free (void)
{
	if (m_partitions != NULL)
		m_partitions->release();
	
	super::free();
}

OSSet*	com_osxkernel_driver_PartitionScheme::scan(SInt32* score)
{
	IOBufferMemoryDescriptor*	buffer			= NULL;
	SamplePartitionTable*		sampleTable;
	IOMedia*					media			= getProvider();
	UInt64						mediaBlockSize	= media->getPreferredBlockSize();
	bool						mediaIsOpen		= false;
	OSSet*						partitions		= NULL;
	IOReturn					status;
	
	// Determine whether this media is formatted.
	if (media->isFormatted() == false)
		goto bail;
	
	// Allocate a sector-sized buffer to hold data read from disk
	buffer = IOBufferMemoryDescriptor::withCapacity(mediaBlockSize, kIODirectionIn);
	if (buffer == NULL)
		goto bail;
	
	// Allocate a set to hold the media objects representing partitions
	partitions = OSSet::withCapacity(8);
	if (partitions == NULL)
		goto bail;
	
	// Open the media with read access
	mediaIsOpen = open(this, 0, kIOStorageAccessReader);
	if (mediaIsOpen == false)
		goto bail;
	
	// Read the first sector of the disk
	status = media->read(this, 0, buffer);
	if (status != kIOReturnSuccess)
		goto bail;
	sampleTable = (SamplePartitionTable*)buffer->getBytesNoCopy();
	
	// Determine whether the protective map signature is present.
	if (strcmp(sampleTable->partitionIdentifier, SamplePartitionIdentifier) != 0)
		goto bail;
	
	// Scan for valid partition entries in the protective map.
	for (unsigned index = 0; index < sampleTable->partitionCount; index++)
	{
		if (isPartitionCorrupt(&sampleTable->partitionEntries[index]))
			goto bail;
		
		IOMedia*	newMedia = instantiateMediaObject(&sampleTable->partitionEntries[index], 1+index);
		if ( newMedia )
		{
			partitions->setObject(newMedia);
			newMedia->release();
		}
	}
	
	// Release temporary resources
	close(this);
	buffer->release();
	
	return partitions;
	
bail:
	// Release all allocated objects
	if ( mediaIsOpen )	close(this);
	if ( partitions )	partitions->release();
	if ( buffer )		buffer->release();
	
	return NULL;
}

IOMedia* com_osxkernel_driver_PartitionScheme::instantiateMediaObject(SamplePartitionEntry* sampleEntry, unsigned index)
{
	IOMedia*	media          = getProvider();
	UInt64		mediaBlockSize = media->getPreferredBlockSize();
	IOMedia*	newMedia;
	
	newMedia = new IOMedia;
	if ( newMedia )
	{
		UInt64		partitionBase, partitionSize;
		
		partitionBase = OSSwapLittleToHostInt64(sampleEntry->blockStart) * mediaBlockSize;
		partitionSize = OSSwapLittleToHostInt64(sampleEntry->blockCount) * mediaBlockSize;
		
		// Initialize the IOMedia object, giving the partition the content hint "Apple_HFS" (an arbitrary choice)
		if ( newMedia->init(partitionBase, partitionSize, mediaBlockSize, media->getAttributes(), false, media->isWritable(), "Apple_HFS"))
		{
			// Set a name for this partition
			char name[24];
			snprintf(name, sizeof(name), "Untitled %d", (int) index);
			newMedia->setName(name);
			
			// Set a location value (the partition number) for this partition
			char location[12];
			snprintf(location, sizeof(location), "%d", (int)index);
			newMedia->setLocation(location);
			
			// Set the "Partition ID" key for this partition
			newMedia->setProperty(kIOMediaPartitionIDKey, index, 32);
		}
		else
		{
			newMedia->release();
			newMedia = 0;
		}
	}
	
	return newMedia;
}
