#ifndef _MYAUDIOENGINE_H_
#define _MYAUDIOENGINE_H_

#include <IOKit/audio/IOAudioEngine.h>

#include "MyAudioDevice.h"

#define MyAudioEngine com_osxkernel_MyAudioEngine

class MyAudioEngine : public IOAudioEngine
{
    OSDeclareDefaultStructors(MyAudioEngine)

public:
    virtual void free();
    
    virtual bool initHardware(IOService *provider);
    virtual void stop(IOService *provider);
    
    virtual IOAudioStream *createNewAudioStream(IOAudioStreamDirection direction, void *sampleBuffer, UInt32 sampleBufferSize);
    
    virtual IOReturn performAudioEngineStart();
    virtual IOReturn performAudioEngineStop();
    
    virtual UInt32 getCurrentSampleFrame();
    
    virtual IOReturn performFormatChange(IOAudioStream *audioStream, const IOAudioStreamFormat *newFormat, const IOAudioSampleRate *newSampleRate);
    
    virtual IOReturn clipOutputSamples(const void *mixBuf, void *sampleBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);
    virtual IOReturn convertInputSamples(const void *sampleBuf, void *destBuf, UInt32 firstSampleFrame, UInt32 numSampleFrames, const IOAudioStreamFormat *streamFormat, IOAudioStream *audioStream);
    
private:
    IOTimerEventSource*     fAudioInterruptSource;
    SInt16                  *fOutputBuffer;
    SInt16					*fInputBuffer;
    
    static void             interruptOccured(OSObject* owner, IOTimerEventSource* sender);
    void                    handleAudioInterrupt();
    
    UInt32                  fInterruptCount;
    
    SInt64                  fNextTimeout;

};

#endif
