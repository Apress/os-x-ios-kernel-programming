#include <IOKit/IOService.h>

class com_osxkernel_driver_IOKitTest : public IOService
{
	OSDeclareDefaultStructors(com_osxkernel_driver_IOKitTest)
	
private:
	IOLock*				m_lock;
	unsigned long		m_devicePowerState;
	SInt32				m_outstandingIO;
	
protected:
	virtual void		powerChangeDone (unsigned long stateNumber);
	
public:	
	virtual void		free (void);
	
	virtual bool		start (IOService* provider);
	virtual void		stop (IOService* provider);
	
	virtual IOReturn	setPowerState (unsigned long powerStateOrdinal, IOService* device);
	
	IOReturn			myReadDataFromDevice ();
	
	
	IOReturn			setProperties (OSObject* properties);
};
