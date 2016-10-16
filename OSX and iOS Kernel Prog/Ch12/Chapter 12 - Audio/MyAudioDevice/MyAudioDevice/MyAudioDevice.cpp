#include "MyAudioDevice.h"

#include <IOKit/audio/IOAudioControl.h>
#include <IOKit/audio/IOAudioLevelControl.h>
#include <IOKit/audio/IOAudioToggleControl.h>
#include <IOKit/audio/IOAudioDefines.h>

#include "MyAudioEngine.h"

#include <IOKit/IOLib.h>

#define super IOAudioDevice

OSDefineMetaClassAndStructors(MyAudioDevice, IOAudioDevice)

bool MyAudioDevice::initHardware(IOService *provider)
{
    bool result = false;
    
    IOLog("MyAudioDevice[%p]::initHardware(%p)\n", this, provider);
    
    if (!super::initHardware(provider))
        goto done;
        
    setDeviceName("My Audio Device");
    setDeviceShortName("MyAudioDevice");
    setManufacturerName("osxkernel.com");
        
    if (!createAudioEngine())
        goto done;
    
    result = true;
    
done:
    return result;
}

#define CREATE_VOLUME_CONTROL(chanID, chanName, usage) \
    IOAudioLevelControl::createVolumeControl(65535, 0, 65535, 0, (-22 << 16) + (32768), chanID, chanName, 0, usage)

#define CREATE_MUTE_CONTROL(usage) \
    IOAudioToggleControl::createMuteControl(false, kIOAudioControlChannelIDAll, kIOAudioControlChannelNameAll, 0, usage)

#define ADD_CONTROL(control, callback) \
    if (!control) \
        goto done; \
    control->setValueChangeHandler(callback, this); \
    audioEngine->addDefaultAudioControl(control); \
    control->release();


bool MyAudioDevice::createAudioEngine()
{
    bool result = false;
    MyAudioEngine *audioEngine = NULL;
    IOAudioControl *control;
       
    audioEngine = new MyAudioEngine();
    if (!audioEngine)
        goto done;
    
    if (!audioEngine->init(NULL))
        goto done;
  
    control = CREATE_VOLUME_CONTROL(kIOAudioControlChannelIDDefaultLeft, kIOAudioControlChannelNameLeft, kIOAudioControlUsageOutput);
    ADD_CONTROL(control, volumeChangeHandler)
    
    control = CREATE_VOLUME_CONTROL(kIOAudioControlChannelIDDefaultRight, kIOAudioControlChannelNameRight, kIOAudioControlUsageOutput);
    ADD_CONTROL(control, volumeChangeHandler)
    
    control = CREATE_MUTE_CONTROL(kIOAudioControlUsageOutput);
    ADD_CONTROL(control, outputMuteChangeHandler)
    
    control = CREATE_VOLUME_CONTROL(kIOAudioControlChannelIDDefaultLeft, kIOAudioControlChannelNameLeft, kIOAudioControlUsageInput);
    ADD_CONTROL(control, gainChangeHandler)

    control = CREATE_VOLUME_CONTROL(kIOAudioControlChannelIDDefaultRight, kIOAudioControlChannelNameRight, kIOAudioControlUsageInput);
    ADD_CONTROL(control, gainChangeHandler)

    control = CREATE_MUTE_CONTROL(kIOAudioControlUsageInput);
    ADD_CONTROL(control, inputMuteChangeHandler)
    
    activateAudioEngine(audioEngine);
   	audioEngine->release(); 
    result = true;
    
done:
   
    if (!result && (audioEngine != NULL)) {
        audioEngine->release();
    }

    return result;
}

IOReturn MyAudioDevice::volumeChangeHandler(OSObject *target, IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    MyAudioDevice *audioDevice;
    audioDevice = (MyAudioDevice *)target;
    if (audioDevice)
        result = audioDevice->volumeChanged(volumeControl, oldValue, newValue);
    return result;
}

IOReturn MyAudioDevice::volumeChanged(IOAudioControl *volumeControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("MyAudioDevice[%p]::volumeChanged(%p, %d, %d)\n", this, volumeControl, (int)oldValue, (int)newValue);
    if (volumeControl)
        IOLog("\t-> Channel %u\n", (unsigned int)volumeControl->getChannelID());
        
    return kIOReturnSuccess;
}

IOReturn MyAudioDevice::outputMuteChangeHandler(OSObject *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    MyAudioDevice *audioDevice;
    
    audioDevice = (MyAudioDevice*)target;
    if (audioDevice) {
        result = audioDevice->outputMuteChanged(muteControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn MyAudioDevice::outputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("MyAudioDevice[%p]::outputMuteChanged(%p, %d, %d)\n", this, muteControl, (int)oldValue, (int)newValue);
        
    return kIOReturnSuccess;
}

IOReturn MyAudioDevice::gainChangeHandler(OSObject *target, IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    MyAudioDevice *audioDevice;
    
    audioDevice = (MyAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->gainChanged(gainControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn MyAudioDevice::gainChanged(IOAudioControl *gainControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("MyAudioDevice[%p]::gainChanged(%p, %d, %d)\n", this, gainControl, (int)oldValue, (int)newValue);
    
    if (gainControl) {
        IOLog("\t-> Channel %u\n", (unsigned int)gainControl->getChannelID());
    }
        
    return kIOReturnSuccess;
}

IOReturn MyAudioDevice::inputMuteChangeHandler(OSObject *target, IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOReturn result = kIOReturnBadArgument;
    MyAudioDevice *audioDevice;
    
    audioDevice = (MyAudioDevice *)target;
    if (audioDevice) {
        result = audioDevice->inputMuteChanged(muteControl, oldValue, newValue);
    }
    
    return result;
}

IOReturn MyAudioDevice::inputMuteChanged(IOAudioControl *muteControl, SInt32 oldValue, SInt32 newValue)
{
    IOLog("MyAudioDevice[%p]::inputMuteChanged(%p, %d, %d)\n", this, muteControl, (int)oldValue, (int)newValue);
        
    return kIOReturnSuccess;
}

