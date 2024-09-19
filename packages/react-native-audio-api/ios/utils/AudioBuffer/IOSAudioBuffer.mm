#import "IOSAudioBuffer.h"

namespace audioapi {

IOSAudioBuffer::IOSAudioBuffer(RNAA_AudioBuffer *audioBuffer)
{
  audioBuffer_ = audioBuffer;
}

IOSAudioBuffer::~IOSAudioBuffer()
{
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

float *IOSAudioBuffer::getChannelData(int channel)
{
  if (!audioBuffer_) {
    return nullptr;
  }

  float *data = [audioBuffer_ getChannelDataForChannel:channel];
  if (!data) {
    return nullptr;
  }

  return data;
}

void IOSAudioBuffer::setChannelData(int channel, float *data, int length)
{
  if (!audioBuffer_) {
    return;
  }

  if (data && length == audioBuffer_.length) {
    [audioBuffer_ setChannelData:channel data:data length:length];
  } else {
    throw std::invalid_argument("Data length does not match buffer length or data is null");
  }
}

} // namespace audioapi
