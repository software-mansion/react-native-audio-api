#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::function<void(void)> &onError,
    const std::function<void(void)> &onStatusChange,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady)
    : AudioRecorder(sampleRate, bufferLength, onError, onStatusChange, onAudioReady)
{
  AudioReceiverBlock audioReceiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames, AVAudioTime *when) {
    if (isRunning_.load()) {
      auto bus = std::make_shared<AudioBus>(numFrames, 1, sampleRate);

      auto *inputChannel = (float *)inputBuffer->mBuffers[0].mData;
      auto *outputChannel = bus->getChannel(0)->getData();

      memcpy(outputChannel, inputChannel, numFrames * sizeof(float));
      onAudioReady_(bus, numFrames, [when sampleTime] / [when sampleRate]);
    }
  };

  audioRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:audioReceiverBlock bufferLength:bufferLength];
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  AudioRecorder::~AudioRecorder();

  stop();
  [audioRecorder_ cleanup];
}

void IOSAudioRecorder::start()
{
  if (isRunning_.load()) {
    return;
  }

  [audioRecorder_ start];
  isRunning_.store(true);
}

void IOSAudioRecorder::stop()
{
  if (!isRunning_.load()) {
    return;
  }

  isRunning_.store(false);
  [audioRecorder_ stop];
}

} // namespace audioapi
