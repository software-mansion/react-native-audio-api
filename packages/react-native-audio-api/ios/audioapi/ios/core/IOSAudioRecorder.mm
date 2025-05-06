#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady)
    : AudioRecorder(sampleRate, bufferLength, onAudioReady)
{
  circularBuffer_ = std::make_shared<CircularAudioArray>(std::max(2 * bufferLength, 2048));

  AudioReceiverBlock audioReceiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames, AVAudioTime *when) {
    if (isRunning_.load()) {
      auto *inputChannel = static_cast<float *>(inputBuffer->mBuffers[0].mData);
      circularBuffer_->push_back(inputChannel, numFrames);
    }

    while (circularBuffer_->getNumberOfAvailableFrames() >= bufferLength_) {
      auto bus = std::make_shared<AudioBus>(bufferLength_, 1, sampleRate_);
      auto *outputChannel = bus->getChannel(0)->getData();

      circularBuffer_->pop_front(outputChannel, bufferLength_);
      onAudioReady_(bus, bufferLength_, [when sampleTime] / [when sampleRate]);
    }
  };

  audioRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:audioReceiverBlock
                                                         bufferLength:bufferLength
                                                           sampleRate:sampleRate];
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

  sendRemainingData();
}

void IOSAudioRecorder::sendRemainingData()
{
  auto bus = std::make_shared<AudioBus>(circularBuffer_->getNumberOfAvailableFrames(), 1, sampleRate_);
  auto *outputChannel = bus->getChannel(0)->getData();
  auto availableFrames = circularBuffer_->getNumberOfAvailableFrames();

  circularBuffer_->pop_front(outputChannel, circularBuffer_->getNumberOfAvailableFrames());
  onAudioReady_(bus, availableFrames, 0);
}

} // namespace audioapi
