#include <IOKit/storage/IOPartitionScheme.h>

#pragma once

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


class com_osxkernel_driver_PartitionScheme : public IOPartitionScheme
{
	OSDeclareDefaultStructors(com_osxkernel_driver_PartitionScheme)
	
protected:
	OSSet*		m_partitions;
	
	virtual OSSet*		scan(SInt32 * score);
	virtual IOMedia*	instantiateMediaObject(SamplePartitionEntry* sampleEntry, unsigned index);
	bool				isPartitionCorrupt (SamplePartitionEntry* sampleEntry)		{ return false; }
	
public:
	virtual IOService*	probe(IOService* provider, SInt32* score);
	virtual bool		start (IOService* provider);
	virtual void		stop(IOService* provider);
	virtual void		free (void);
	virtual IOReturn	requestProbe(IOOptionBits options);
};
