#include <IOKit/storage/IOFilterScheme.h>

#pragma once

class com_osxkernel_driver_SampleEncryptionFilter : public IOFilterScheme
{
	OSDeclareDefaultStructors(com_osxkernel_driver_SampleEncryptionFilter)
	
protected:
	IOMedia*		m_encryptedMedia;
	IOMedia*		m_childMedia;
	
	struct ReadCompletionParams {
		IOStorageCompletion		completion;
		IOMemoryDescriptor*		buffer;
	};
	
	IOMedia*			instantiateMediaObject ();
	static void			readCompleted (void* target, void* parameter, IOReturn status, UInt64 actualByteCount);
	
public:
	virtual bool		start (IOService* provider);
	virtual void		stop (IOService* provider);
	virtual void		free (void);
	
	virtual void		read (IOService* client, UInt64 byteStart, IOMemoryDescriptor* buffer, IOStorageAttributes* attributes, IOStorageCompletion* completion);
	virtual void		write (IOService* client, UInt64 byteStart, IOMemoryDescriptor* buffer, IOStorageAttributes* attributes, IOStorageCompletion* completion);
	
	static IOReturn				decryptBuffer (IOMemoryDescriptor* buffer, UInt64 actualByteCount);
	static IOMemoryDescriptor*	encryptBuffer (IOMemoryDescriptor* buffer);
};
