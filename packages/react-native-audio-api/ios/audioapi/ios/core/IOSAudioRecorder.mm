#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <unordered_map>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(sampleRate, bufferLength, audioEventHandlerRegistry)
{
  AudioReceiverBlock audioReceiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames) {
    if (isRunning_.load()) {
      auto *inputChannel = static_cast<float *>(inputBuffer->mBuffers[0].mData);
      writeToBuffers(inputChannel, numFrames);
    }

    while (circularBuffer_->getNumberOfAvailableFrames() >= bufferLength_) {
      auto bus = std::make_shared<AudioBus>(bufferLength_, 1, sampleRate_);
      auto *outputChannel = bus->getChannel(0)->getData();

      circularBuffer_->pop_front(outputChannel, bufferLength_);

      invokeOnAudioReadyCallback(bus, bufferLength_);
    }
  };

  audioRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:audioReceiverBlock
                                                         bufferLength:bufferLength
                                                           sampleRate:sampleRate];
}

IOSAudioRecorder::~IOSAudioRecorder()
{
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

  sendRemainingData();
}

} // namespace audioapi
