#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/WsolaTimeStretcher.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AudioBufferBaseSourceNodeHostObject::AudioBufferBaseSourceNodeHostObject(
    const std::shared_ptr<utils::graph::Graph> &graph,
    std::unique_ptr<AudioNode> node,
    const BaseAudioBufferSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(graph, std::move(node), options),
      bufferBaseSourceNode_(typedAudioNode<AudioBufferBaseSourceNode>(node_)),
      onPositionChangedInterval_(options.onPositionChangedInterval),
      pitchCorrection_(options.pitchCorrection) {
  detuneParam_ = std::make_shared<AudioParamHostObject>(
      graph_, node_, bufferBaseSourceNode_->getDetuneParam());
  playbackRateParam_ = std::make_shared<AudioParamHostObject>(
      graph_, node_, bufferBaseSourceNode_->getPlaybackRateParam());

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, detune),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, playbackRate),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferBaseSourceNodeHostObject, onPositionChanged),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval));

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioBufferBaseSourceNodeHostObject, getInputLatency),
      JSI_EXPORT_FUNCTION(AudioBufferBaseSourceNodeHostObject, getOutputLatency));
}

AudioBufferBaseSourceNodeHostObject::~AudioBufferBaseSourceNodeHostObject() {
  bufferBaseSourceNode_->assignOnPositionChangedCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, detune) {
  return jsi::Object::createFromHostObject(runtime, detuneParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, playbackRate) {
  return jsi::Object::createFromHostObject(runtime, playbackRateParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval) {
  return {onPositionChangedInterval_};
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChanged) {
  bufferBaseSourceNode_->assignOnPositionChangedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval) {
  auto handle = node_->handle;
  auto interval = static_cast<int>(value.getNumber());

  auto event = [handle, node = bufferBaseSourceNode_, interval](BaseAudioContext &) {
    node->setOnPositionChangedInterval(interval);
  };

  bufferBaseSourceNode_->scheduleAudioEvent(std::move(event));
  onPositionChangedInterval_ = interval;
}

JSI_HOST_FUNCTION_IMPL(AudioBufferBaseSourceNodeHostObject, getInputLatency) {
  return {inputLatency_};
}

JSI_HOST_FUNCTION_IMPL(AudioBufferBaseSourceNodeHostObject, getOutputLatency) {
  return {outputLatency_};
}

void AudioBufferBaseSourceNodeHostObject::initStretch(int channelCount, float sampleRate) {
  auto handle = node_->handle;
  inputLatency_ = WsolaTimeStretcher::INPUT_LATENCY_MS / 1000.0;
  outputLatency_ = WsolaTimeStretcher::OUTPUT_LATENCY_MS / 1000.0;

  auto playbackRateBuffer = std::make_shared<DSPAudioBuffer>(
      WsolaTimeStretcher::MAX_PLAYBACK_RATE * RENDER_QUANTUM_SIZE, channelCount, sampleRate);

  auto event = [handle, node = bufferBaseSourceNode_, playbackRateBuffer, channelCount, sampleRate](
                   BaseAudioContext &) {
    node->initStretch(static_cast<size_t>(channelCount), sampleRate, playbackRateBuffer);
  };
  bufferBaseSourceNode_->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
