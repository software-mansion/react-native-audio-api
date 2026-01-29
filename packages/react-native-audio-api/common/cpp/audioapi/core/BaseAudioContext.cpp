#include <audioapi/HostObjects/utils/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/core/effects/IIRFilterNode.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/core/effects/WorkletNode.h>
#include <audioapi/core/effects/WorkletProcessingNode.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/core/sources/StreamerNode.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/CircularAudioArray.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

BaseAudioContext::BaseAudioContext(
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry)
    : state_(ContextState::SUSPENDED),
      sampleRate_(sampleRate),
      graphManager_(std::make_shared<AudioGraphManager>()),
      audioEventHandlerRegistry_(audioEventHandlerRegistry),
      runtimeRegistry_(runtimeRegistry) {}

void BaseAudioContext::initialize() {
  destination_ = std::make_shared<AudioDestinationNode>(shared_from_this());
}

ContextState BaseAudioContext::getState() {
  auto state = state_.load(std::memory_order_acquire);

  if (state == ContextState::CLOSED || isDriverRunning()) {
    return state;
  }

  return ContextState::SUSPENDED;
}

float BaseAudioContext::getSampleRate() const {
  return sampleRate_.load(std::memory_order_acquire);
}

std::size_t BaseAudioContext::getCurrentSampleFrame() const {
  assert(destination_ != nullptr);
  return destination_->getCurrentSampleFrame();
}

double BaseAudioContext::getCurrentTime() const {
  assert(destination_ != nullptr);
  return destination_->getCurrentTime();
}

std::shared_ptr<AudioDestinationNode> BaseAudioContext::getDestination() const {
  return destination_;
}

void BaseAudioContext::setState(audioapi::ContextState state) {
    state_.store(state, std::memory_order_release);
}

std::shared_ptr<WorkletSourceNode> BaseAudioContext::createWorkletSourceNode(
    std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime,
    bool shouldLockRuntime) {
  WorkletsRunner workletRunner(runtime, shareableWorklet, shouldLockRuntime);
  auto workletSourceNode =
      std::make_shared<WorkletSourceNode>(shared_from_this(), std::move(workletRunner));
  graphManager_->addSourceNode(workletSourceNode);
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
      shared_from_this(), bufferLength, inputChannelCount, std::move(workletRunner));
  graphManager_->addProcessingNode(workletNode);
  return workletNode;
}

std::shared_ptr<WorkletProcessingNode> BaseAudioContext::createWorkletProcessingNode(
    std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime,
    bool shouldLockRuntime) {
  WorkletsRunner workletRunner(runtime, shareableWorklet, shouldLockRuntime);
  auto workletProcessingNode =
      std::make_shared<WorkletProcessingNode>(shared_from_this(), std::move(workletRunner));
  graphManager_->addProcessingNode(workletProcessingNode);
  return workletProcessingNode;
}

std::shared_ptr<RecorderAdapterNode> BaseAudioContext::createRecorderAdapter() {
  auto recorderAdapter = std::make_shared<RecorderAdapterNode>(shared_from_this());
  graphManager_->addProcessingNode(recorderAdapter);
  return recorderAdapter;
}

std::shared_ptr<OscillatorNode> BaseAudioContext::createOscillator(const OscillatorOptions &options) {
  auto oscillator = std::make_shared<OscillatorNode>(shared_from_this(), options);
  graphManager_->addSourceNode(oscillator);
  return oscillator;
}

std::shared_ptr<ConstantSourceNode> BaseAudioContext::createConstantSource(
    const ConstantSourceOptions &options) {
  auto constantSource = std::make_shared<ConstantSourceNode>(shared_from_this(), options);
  graphManager_->addSourceNode(constantSource);
  return constantSource;
}

std::shared_ptr<StreamerNode> BaseAudioContext::createStreamer(const StreamerOptions &options) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  auto streamer = std::make_shared<StreamerNode>(shared_from_this(), options);
  graphManager_->addSourceNode(streamer);
  return streamer;
#else
  return nullptr;
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

std::shared_ptr<GainNode> BaseAudioContext::createGain(const GainOptions &options) {
  auto gain = std::make_shared<GainNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(gain);
  return gain;
}

std::shared_ptr<StereoPannerNode> BaseAudioContext::createStereoPanner(
    const StereoPannerOptions &options) {
  auto stereoPanner = std::make_shared<StereoPannerNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(stereoPanner);
  return stereoPanner;
}

std::shared_ptr<DelayNode> BaseAudioContext::createDelay(const DelayOptions &options) {
  auto delay = std::make_shared<DelayNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(delay);
  return delay;
}

std::shared_ptr<BiquadFilterNode> BaseAudioContext::createBiquadFilter(
    const BiquadFilterOptions &options) {
  auto biquadFilter = std::make_shared<BiquadFilterNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(biquadFilter);
  return biquadFilter;
}

std::shared_ptr<AudioBufferSourceNode> BaseAudioContext::createBufferSource(
    const AudioBufferSourceOptions &options) {
  auto bufferSource = std::make_shared<AudioBufferSourceNode>(shared_from_this(), options);
  graphManager_->addSourceNode(bufferSource);
  return bufferSource;
}

std::shared_ptr<IIRFilterNode> BaseAudioContext::createIIRFilter(const IIRFilterOptions &options) {
  auto iirFilter = std::make_shared<IIRFilterNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(iirFilter);
  return iirFilter;
}

std::shared_ptr<AudioBufferQueueSourceNode> BaseAudioContext::createBufferQueueSource(
    const BaseAudioBufferSourceOptions &options) {
  auto bufferSource = std::make_shared<AudioBufferQueueSourceNode>(shared_from_this(), options);
  graphManager_->addSourceNode(bufferSource);
  return bufferSource;
}

std::shared_ptr<AudioBuffer> BaseAudioContext::createBuffer(const AudioBufferOptions &options) {
  return std::make_shared<AudioBuffer>(options);
}

std::shared_ptr<PeriodicWave> BaseAudioContext::createPeriodicWave(
    const std::vector<std::complex<float>> &complexData,
    bool disableNormalization,
    int length) const {
  return std::make_shared<PeriodicWave>(getSampleRate(), complexData, length, disableNormalization);
}

std::shared_ptr<AnalyserNode> BaseAudioContext::createAnalyser(const AnalyserOptions &options) {
  auto analyser = std::make_shared<AnalyserNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(analyser);
  return analyser;
}

std::shared_ptr<ConvolverNode> BaseAudioContext::createConvolver(const ConvolverOptions &options) {
  auto convolver = std::make_shared<ConvolverNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(convolver);
  return convolver;
}

std::shared_ptr<WaveShaperNode> BaseAudioContext::createWaveShaper(const WaveShaperOptions &options) {
  auto waveShaper = std::make_shared<WaveShaperNode>(shared_from_this(), options);
  graphManager_->addProcessingNode(waveShaper);
  return waveShaper;
}

float BaseAudioContext::getNyquistFrequency() const {
  return getSampleRate() / 2.0f;
}

std::shared_ptr<PeriodicWave> BaseAudioContext::getBasicWaveForm(OscillatorType type) {
  switch (type) {
    case OscillatorType::SINE:
      if (cachedSineWave_ == nullptr) {
        cachedSineWave_ = std::make_shared<PeriodicWave>(getSampleRate(), type, false);
      }
      return cachedSineWave_;
    case OscillatorType::SQUARE:
      if (cachedSquareWave_ == nullptr) {
        cachedSquareWave_ = std::make_shared<PeriodicWave>(getSampleRate(), type, false);
      }
      return cachedSquareWave_;
    case OscillatorType::SAWTOOTH:
      if (cachedSawtoothWave_ == nullptr) {
        cachedSawtoothWave_ = std::make_shared<PeriodicWave>(getSampleRate(), type, false);
      }
      return cachedSawtoothWave_;
    case OscillatorType::TRIANGLE:
      if (cachedTriangleWave_ == nullptr) {
        cachedTriangleWave_ = std::make_shared<PeriodicWave>(getSampleRate(), type, false);
      }
      return cachedTriangleWave_;
    case OscillatorType::CUSTOM:
      throw std::invalid_argument("You can't get a custom wave form. You need to create it.");
      break;
  }
}

std::shared_ptr<AudioGraphManager> BaseAudioContext::getGraphManager() const {
  return graphManager_;
}

std::shared_ptr<IAudioEventHandlerRegistry> BaseAudioContext::getAudioEventHandlerRegistry() const {
  return audioEventHandlerRegistry_;
}

const RuntimeRegistry &BaseAudioContext::getRuntimeRegistry() const {
    return runtimeRegistry_;
}

} // namespace audioapi
