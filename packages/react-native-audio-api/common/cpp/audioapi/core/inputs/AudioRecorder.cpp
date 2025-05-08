#include <audioapi/HostObjects/AudioBufferHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

namespace audioapi {

AudioRecorder::AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      audioEventHandlerRegistry_(audioEventHandlerRegistry) {}

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

  std::unordered_map<std::string, Value> body = {};
  body.insert({"buffer", audioBufferHostObject});
  body.insert({"numFrames", numFrames});
  body.insert({"when", when});

  audioEventHandlerRegistry_->invokeHandlerWithEventBody(
      "audioReady", onAudioReadyCallbackId_, body);
}

} // namespace audioapi
