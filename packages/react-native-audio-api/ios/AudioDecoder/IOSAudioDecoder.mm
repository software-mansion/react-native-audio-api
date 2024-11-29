#import <AVFoundation/AVFoundation.h>

#include <AudioArray.h>
#include <AudioBus.h>
#include <IOSAudioDecoder.h>

namespace audioapi {

IOSAudioDecoder::IOSAudioDecoder(int sampleRate) : sampleRate_(sampleRate)
{
  audioDecoder_ = [[AudioDecoder alloc] initWithSampleRate:sampleRate_];
}

IOSAudioDecoder::~IOSAudioDecoder()
{
  [audioDecoder_ cleanup];
  audioDecoder_ = nullptr;
}

AudioBus *IOSAudioDecoder::decodeWithFilePath(std::string path)
{
  auto bufferList = [audioDecoder_ decodeWithFilePath:[NSString stringWithUTF8String:path.c_str()]];
  auto numberOfChannels = bufferList->mNumberBuffers;
  auto size = bufferList->mBuffers[0].mDataByteSize / sizeof(float);

  auto audioBus = new AudioBus(sampleRate_, size, numberOfChannels);

  for (int i = 0; i < numberOfChannels; i++) {
    float *data = (float *)bufferList->mBuffers[i].mData;
    memcpy(audioBus->getChannel(i)->getData(), data, sizeof(float) * size);
  }

  return audioBus;
}
} // namespace audioapi
