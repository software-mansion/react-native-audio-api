#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <unordered_map>

namespace audioapi {

IOSAudioRecorder::IOSAudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(sampleRate, bufferLength, audioEventHandlerRegistry)
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
      
      auto audioBuffer = std::make_shared<AudioBuffer>(bus);
      auto audioBufferHostObject =
          std::make_shared<AudioBufferHostObject>(audioBuffer);

      std::unordered_map<std::string, Value> body = {};
      body.insert({"buffer", audioBufferHostObject});
      body.insert({"numFrames", bufferLength_});
      body.insert({"when", [when sampleTime] / [when sampleRate]});
      
      audioEventHandlerRegistry_->invokeHandlerWithEventBody(
          "audioReady", onAudioReadyCallbackId_, body);
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
  auto availableFrames = static_cast<int>(circularBuffer_->getNumberOfAvailableFrames());

  circularBuffer_->pop_front(outputChannel, circularBuffer_->getNumberOfAvailableFrames());
  
  auto audioBuffer = std::make_shared<AudioBuffer>(bus);
  auto audioBufferHostObject =
      std::make_shared<AudioBufferHostObject>(audioBuffer);
  
  std::unordered_map<std::string, Value> body = {};
  body.insert({"buffer", audioBufferHostObject});
  body.insert({"numFrames", availableFrames});
  body.insert({"when", 0});
  
  audioEventHandlerRegistry_->invokeHandlerWithEventBody(
      "audioReady", onAudioReadyCallbackId_, body);
}

} // namespace audioapi
