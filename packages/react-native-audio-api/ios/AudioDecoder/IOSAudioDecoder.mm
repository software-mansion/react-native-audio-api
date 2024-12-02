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

AudioBus *IOSAudioDecoder::decode(const std::string &path)
{
  auto bufferList = [audioDecoder_ decode:[NSString stringWithUTF8String:path.c_str()]];
  AudioBus *audioBus;
  if (bufferList) {
    auto numberOfChannels = bufferList->mNumberBuffers;
    auto size = bufferList->mBuffers[0].mDataByteSize / sizeof(float);

    audioBus = new AudioBus(sampleRate_, size, numberOfChannels);

    for (int i = 0; i < numberOfChannels; i++) {
      float *data = (float *)bufferList->mBuffers[i].mData;
      memcpy(audioBus->getChannel(i)->getData(), data, sizeof(float) * size);
    }
  } else {
    audioBus = new AudioBus(sampleRate_, 1, 1);
  }

  return audioBus;
}
} // namespace audioapi
