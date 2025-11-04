#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/core/effects/WorkletNode.h>
#include <audioapi/core/effects/WorkletProcessingNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/sources/StreamerNode.h>
#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>

namespace audioapi {

BaseAudioContext::BaseAudioContext(
    const std::shared_ptr<IAudioEventHandlerRegistry>
        &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry) {
  nodeManager_ = std::make_shared<AudioNodeManager>();
  destination_ = std::make_shared<AudioDestinationNode>(this);

  audioEventHandlerRegistry_ = audioEventHandlerRegistry;
  runtimeRegistry_ = runtimeRegistry;
}

std::string BaseAudioContext::getState() {
  if (isDriverRunning()) {
    return BaseAudioContext::toString(state_);
  }

  if (state_ == ContextState::CLOSED) {
    return BaseAudioContext::toString(ContextState::CLOSED);
  }

  return BaseAudioContext::toString(ContextState::SUSPENDED);
}

float BaseAudioContext::getSampleRate() const {
  return sampleRate_;
}

std::size_t BaseAudioContext::getCurrentSampleFrame() const {
  assert(destination_ != nullptr);
  return destination_->getCurrentSampleFrame();
}

double BaseAudioContext::getCurrentTime() const {
  assert(destination_ != nullptr);
  return destination_->getCurrentTime();
}

std::shared_ptr<AudioDestinationNode> BaseAudioContext::getDestination() {
  return destination_;
}

std::shared_ptr<WorkletSourceNode> BaseAudioContext::createWorkletSourceNode(
    std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime,
    bool shouldLockRuntime) {
  WorkletsRunner workletRunner(runtime, shareableWorklet, shouldLockRuntime);
  auto workletSourceNode =
      std::make_shared<WorkletSourceNode>(this, std::move(workletRunner));
  nodeManager_->addSourceNode(workletSourceNode);
  return workletSourceNode;
}

std::shared_ptr<WorkletNode> BaseAudioContext::createWorkletNode(
    std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime,
    size_t bufferLength,
    size_t inputChannelCount,
    bool shouldLockRuntime) {
  WorkletsRunner workletRunner(runtime, shareableWorklet, shouldLockRuntime);
  auto workletNode = std::make_shared<WorkletNode>(
      this, bufferLength, inputChannelCount, std::move(workletRunner));
  nodeManager_->addProcessingNode(workletNode);
  return workletNode;
}

std::shared_ptr<WorkletProcessingNode>
BaseAudioContext::createWorkletProcessingNode(
    std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime,
    bool shouldLockRuntime) {
  WorkletsRunner workletRunner(runtime, shareableWorklet, shouldLockRuntime);
  auto workletProcessingNode =
      std::make_shared<WorkletProcessingNode>(this, std::move(workletRunner));
  nodeManager_->addProcessingNode(workletProcessingNode);
  return workletProcessingNode;
}

std::shared_ptr<RecorderAdapterNode> BaseAudioContext::createRecorderAdapter() {
  auto recorderAdapter = std::make_shared<RecorderAdapterNode>(this);
  nodeManager_->addProcessingNode(recorderAdapter);
  return recorderAdapter;
}

std::shared_ptr<OscillatorNode> BaseAudioContext::createOscillator() {
  auto oscillator = std::make_shared<OscillatorNode>(this);
  nodeManager_->addSourceNode(oscillator);
  return oscillator;
}

std::shared_ptr<ConstantSourceNode> BaseAudioContext::createConstantSource() {
  auto constantSource = std::make_shared<ConstantSourceNode>(this);
  nodeManager_->addSourceNode(constantSource);
  return constantSource;
}

#ifndef AUDIO_API_TEST_SUITE
std::shared_ptr<StreamerNode> BaseAudioContext::createStreamer() {
  auto streamer = std::make_shared<StreamerNode>(this);
  nodeManager_->addSourceNode(streamer);
  return streamer;
}
#endif

std::shared_ptr<GainNode> BaseAudioContext::createGain() {
  auto gain = std::make_shared<GainNode>(this);
  nodeManager_->addProcessingNode(gain);
  return gain;
}

std::shared_ptr<StereoPannerNode> BaseAudioContext::createStereoPanner() {
  auto stereoPanner = std::make_shared<StereoPannerNode>(this);
  nodeManager_->addProcessingNode(stereoPanner);
  return stereoPanner;
}

std::shared_ptr<BiquadFilterNode> BaseAudioContext::createBiquadFilter() {
  auto biquadFilter = std::make_shared<BiquadFilterNode>(this);
  nodeManager_->addProcessingNode(biquadFilter);
  return biquadFilter;
}

std::shared_ptr<AudioBufferSourceNode> BaseAudioContext::createBufferSource(
    bool pitchCorrection) {
  auto bufferSource =
      std::make_shared<AudioBufferSourceNode>(this, pitchCorrection);
  nodeManager_->addSourceNode(bufferSource);
  return bufferSource;
}

std::shared_ptr<AudioBufferQueueSourceNode>
BaseAudioContext::createBufferQueueSource(bool pitchCorrection) {
  auto bufferSource =
      std::make_shared<AudioBufferQueueSourceNode>(this, pitchCorrection);
  nodeManager_->addSourceNode(bufferSource);
  return bufferSource;
}

std::shared_ptr<AudioBuffer> BaseAudioContext::createBuffer(
    int numberOfChannels,
    size_t length,
    float sampleRate) {
  return std::make_shared<AudioBuffer>(numberOfChannels, length, sampleRate);
}

std::shared_ptr<PeriodicWave> BaseAudioContext::createPeriodicWave(
    const std::vector<std::complex<float>> &complexData,
    bool disableNormalization,
    int length) {
  return std::make_shared<PeriodicWave>(
      sampleRate_, complexData, length, disableNormalization);
}

std::shared_ptr<AnalyserNode> BaseAudioContext::createAnalyser() {
  auto analyser = std::make_shared<AnalyserNode>(this);
  nodeManager_->addProcessingNode(analyser);
  return analyser;
}

std::shared_ptr<ConvolverNode> BaseAudioContext::createConvolver(
    std::shared_ptr<AudioBuffer> buffer,
    bool disableNormalization) {
  auto convolver =
      std::make_shared<ConvolverNode>(this, buffer, disableNormalization);
  nodeManager_->addProcessingNode(convolver);
  return convolver;
}

AudioNodeManager *BaseAudioContext::getNodeManager() {
  return nodeManager_.get();
}

bool BaseAudioContext::isRunning() const {
  return state_ == ContextState::RUNNING && isDriverRunning();
}

bool BaseAudioContext::isSuspended() const {
  return state_ == ContextState::SUSPENDED || !isDriverRunning();
}

bool BaseAudioContext::isClosed() const {
  return state_ == ContextState::CLOSED;
}

float BaseAudioContext::getNyquistFrequency() const {
  return sampleRate_ / 2.0f;
}

std::string BaseAudioContext::toString(ContextState state) {
  switch (state) {
    case ContextState::SUSPENDED:
      return "suspended";
    case ContextState::RUNNING:
      return "running";
    case ContextState::CLOSED:
      return "closed";
    default:
      throw std::invalid_argument("Unknown context state");
  }
}

std::shared_ptr<PeriodicWave> BaseAudioContext::getBasicWaveForm(
    OscillatorType type) {
  switch (type) {
    case OscillatorType::SINE:
      if (cachedSineWave_ == nullptr) {
        cachedSineWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedSineWave_;
    case OscillatorType::SQUARE:
      if (cachedSquareWave_ == nullptr) {
        cachedSquareWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedSquareWave_;
    case OscillatorType::SAWTOOTH:
      if (cachedSawtoothWave_ == nullptr) {
        cachedSawtoothWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedSawtoothWave_;
    case OscillatorType::TRIANGLE:
      if (cachedTriangleWave_ == nullptr) {
        cachedTriangleWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedTriangleWave_;
    case OscillatorType::CUSTOM:
      throw std::invalid_argument(
          "You can't get a custom wave form. You need to create it.");
      break;
  }
}

} // namespace audioapi
