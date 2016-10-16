#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

class com_osxkernel_driver_RAMDisk : public IOService
{
	OSDeclareDefaultStructors(com_osxkernel_driver_RAMDisk)
	
private:
	IOBufferMemoryDescriptor*		m_memoryDesc;
	void*							m_buffer;
	
protected:
	bool			createBlockStorageDevice ();
	
public:
	virtual bool	start (IOService* provider);
	virtual void	free (void);
	
	virtual IOByteCount		performRead (IOMemoryDescriptor* dstDesc, UInt64 byteOffset, UInt64 byteCount);
	virtual IOByteCount		performWrite (IOMemoryDescriptor* srcDesc, UInt64 byteOffset, UInt64 byteCount);
};
