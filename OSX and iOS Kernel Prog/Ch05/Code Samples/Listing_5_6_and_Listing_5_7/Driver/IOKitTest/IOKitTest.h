#include <IOKit/IOService.h>

class com_osxkernel_driver_IOKitTest : public IOService
{
	OSDeclareDefaultStructors(com_osxkernel_driver_IOKitTest)
	
public:	
	virtual bool	init (OSDictionary* dictionary = NULL);
	virtual void	free (void);
	
	virtual IOService*	probe (IOService* provider, SInt32* score);
	virtual bool	start (IOService* provider);
	virtual void	stop (IOService* provider);
	
	virtual IOReturn	setProperties (OSObject* properties);
};
