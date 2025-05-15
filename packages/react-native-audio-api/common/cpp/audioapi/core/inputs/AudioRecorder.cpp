#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>

namespace audioapi {

AudioRecorder::AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      audioEventHandlerRegistry_(audioEventHandlerRegistry) {
  circularBuffer_ =
      std::make_shared<CircularAudioArray>(std::max(2 * bufferLength, 2048));
  isRunning_.store(false);
}

void AudioRecorder::setOnAudioReadyCallbackId(uint64_t callbackId) {
  onAudioReadyCallbackId_ = callbackId;
}

void AudioRecorder::invokeOnAudioReadyCallback(
    const std::shared_ptr<AudioBus> &bus,
    int numFrames,
    double when) {
  auto audioBuffer = std::make_shared<AudioBuffer>(bus);
  auto audioBufferHostObject =
      std::make_shared<AudioBufferHostObject>(audioBuffer);

  std::unordered_map<std::string, EventValue> body = {};
  body.insert({"buffer", audioBufferHostObject});
  body.insert({"numFrames", numFrames});
  body.insert({"when", when});

  audioEventHandlerRegistry_->invokeHandlerWithEventBody(
      "audioReady", onAudioReadyCallbackId_, body);
}

void AudioRecorder::sendRemainingData() {
  auto bus = std::make_shared<AudioBus>(
      circularBuffer_->getNumberOfAvailableFrames(), 1, sampleRate_);
  auto *outputChannel = bus->getChannel(0)->getData();
  auto availableFrames =
      static_cast<int>(circularBuffer_->getNumberOfAvailableFrames());

  circularBuffer_->pop_front(
      outputChannel, circularBuffer_->getNumberOfAvailableFrames());

  invokeOnAudioReadyCallback(bus, availableFrames, 0);
}

} // namespace audioapi
