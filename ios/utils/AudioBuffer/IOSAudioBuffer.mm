#import "IOSAudioBuffer.h"

namespace audioapi {

IOSAudioBuffer::IOSAudioBuffer(int sampleRate, int length, int numberOfChannels)
{
  audioBuffer_ = [[RNAA_AudioBuffer alloc] initWithSampleRate:sampleRate
                                                       length:length
                                             numberOfChannels:numberOfChannels];
}

IOSAudioBuffer::~IOSAudioBuffer()
{
  [audioBuffer_ cleanup];
  audioBuffer_ = nullptr;
}

int IOSAudioBuffer::getSampleRate()
{
  return audioBuffer_.sampleRate;
}

int IOSAudioBuffer::getLength()
{
  return audioBuffer_.length;
}

int IOSAudioBuffer::getNumberOfChannels()
{
  return audioBuffer_.numberOfChannels;
}

float IOSAudioBuffer::getDuration()
{
  return audioBuffer_.duration;
}

float **IOSAudioBuffer::getChannelData(int channel)
{
  @try {
    return [audioBuffer_ getChannelDataForChannel:channel];
  } @catch (NSException *exception) {
    NSLog(@"Exception: %@", exception);
  }
}

void IOSAudioBuffer::setChannelData(int channel, float **data, int length)
{
  @try {
    [audioBuffer_ setChannelData:channel data:data length:length];
  } @catch (NSException *exception) {
    NSLog(@"Exception: %@", exception);
  }
}

} // namespace audioapi
