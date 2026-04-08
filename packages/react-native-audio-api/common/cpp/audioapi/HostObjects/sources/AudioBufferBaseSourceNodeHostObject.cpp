#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace audioapi {

AudioBufferBaseSourceNodeHostObject::AudioBufferBaseSourceNodeHostObject(
    const std::shared_ptr<AudioBufferBaseSourceNode> &node,
    const BaseAudioBufferSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(node, options),
      onPositionChangedInterval_(options.onPositionChangedInterval),
      pitchCorrection_(options.pitchCorrection) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  detuneParam_ = std::make_shared<AudioParamHostObject>(sourceNode->getDetuneParam());
  playbackRateParam_ = std::make_shared<AudioParamHostObject>(sourceNode->getPlaybackRateParam());

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
  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  setOnPositionChangedCallbackId(0);
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
  auto callbackId = std::stoull(value.getString(runtime).utf8(runtime));
  setOnPositionChangedCallbackId(callbackId);
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferBaseSourceNodeHostObject, onPositionChangedInterval) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  auto interval = static_cast<int>(value.getNumber());

  auto event = [sourceNode, interval](BaseAudioContext &) {
    sourceNode->setOnPositionChangedInterval(interval);
  };

  sourceNode->scheduleAudioEvent(std::move(event));
  onPositionChangedInterval_ = interval;
}

JSI_HOST_FUNCTION_IMPL(AudioBufferBaseSourceNodeHostObject, getInputLatency) {
  return {inputLatency_};
}

JSI_HOST_FUNCTION_IMPL(AudioBufferBaseSourceNodeHostObject, getOutputLatency) {
  return {outputLatency_};
}

void AudioBufferBaseSourceNodeHostObject::setOnPositionChangedCallbackId(uint64_t callbackId) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);

  auto event = [sourceNode, callbackId](BaseAudioContext &) {
    sourceNode->setOnPositionChangedCallbackId(callbackId);
  };

  sourceNode->unregisterOnPositionChangedCallback(onPositionChangedCallbackId_);
  sourceNode->scheduleAudioEvent(std::move(event));
  onPositionChangedCallbackId_ = callbackId;
}

void AudioBufferBaseSourceNodeHostObject::initStretch(int channelCount, float sampleRate) {
  auto sourceNode = std::static_pointer_cast<AudioBufferBaseSourceNode>(node_);
  auto stretch = std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>();
  stretch->presetDefault(channelCount, sampleRate);
  inputLatency_ =
      std::max(dsp::sampleFrameToTime(stretch->inputLatency(), node_->getContextSampleRate()), 0.0);
  outputLatency_ = std::max(
      dsp::sampleFrameToTime(stretch->outputLatency(), node_->getContextSampleRate()), 0.0);

  auto playbackRateBuffer =
      std::make_shared<DSPAudioBuffer>(3 * RENDER_QUANTUM_SIZE, channelCount, sampleRate);

  auto event = [sourceNode, stretch, playbackRateBuffer](BaseAudioContext &) {
    sourceNode->initStretch(stretch, playbackRateBuffer);
  };
  sourceNode->scheduleAudioEvent(std::move(event));
}

} // namespace audioapi
