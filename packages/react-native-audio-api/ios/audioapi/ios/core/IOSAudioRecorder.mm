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
  circularBuffer_ = std::make_shared<AudioBus>(std::max(2 * bufferLength, 2048), 1, sampleRate);

  AudioReceiverBlock audioReceiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames, AVAudioTime *when) {
    if (isRunning_.load()) {
      // copying to circularBuffer_
      auto *circularBufferChannel = circularBuffer_->getChannel(0)->getData();
      auto *inputChannel = (float *)inputBuffer->mBuffers[0].mData;

      auto framesToProcess = numFrames;
      auto framesToCopy = 0;

      if (writeIdx_ + numFrames > circularBuffer_->getSize()) {
        framesToCopy = circularBuffer_->getSize() - writeIdx_;
        memcpy(circularBufferChannel + writeIdx_, inputChannel, framesToCopy * sizeof(float));
        framesToProcess -= framesToCopy;
        writeIdx_ = 0;
      }

      memcpy(circularBufferChannel + writeIdx_, inputChannel + framesToCopy, framesToProcess * sizeof(float));

      writeIdx_ += framesToProcess;
      if (writeIdx_ >= circularBuffer_->getSize()) {
        writeIdx_ = 0;
      }

      // copying to output bus and invoking callback
      auto availableFrames =
          writeIdx_ >= readIdx_ ? writeIdx_ - readIdx_ : circularBuffer_->getSize() - (readIdx_ - writeIdx_);

      while (availableFrames >= bufferLength_) {
        auto bus = std::make_shared<AudioBus>(bufferLength_, 1, sampleRate_);
        auto *outputChannel = bus->getChannel(0)->getData();

        framesToProcess = bufferLength_;
        framesToCopy = 0;
        if (readIdx_ + bufferLength_ > circularBuffer_->getSize()) {
          framesToCopy = circularBuffer_->getSize() - readIdx_;
          memcpy(outputChannel, circularBufferChannel + readIdx_, framesToCopy * sizeof(float));
          framesToProcess -= framesToCopy;
          readIdx_ = 0;
        }

        memcpy(outputChannel + framesToCopy, circularBufferChannel + readIdx_, framesToProcess * sizeof(float));

        readIdx_ += framesToProcess;
        if (readIdx_ >= circularBuffer_->getSize()) {
          readIdx_ = 0;
        }

        onAudioReady_(bus, bufferLength_, [when sampleTime] / [when sampleRate]);

        availableFrames -= bufferLength_;
      }
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
}

} // namespace audioapi
