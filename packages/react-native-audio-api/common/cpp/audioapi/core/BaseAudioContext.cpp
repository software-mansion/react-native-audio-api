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
#include <audioapi/core/sources/AudioBufferQueueSourceNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/types/NodeOptions.h>
#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/core/sources/StreamerNode.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/core/sources/WorkletSourceNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/AudioGraphManager.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/CircularArray.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

BaseAudioContext::BaseAudioContext(
    float sampleRate,
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const RuntimeRegistry &runtimeRegistry)
    : state_(ContextState::SUSPENDED),
      sampleRate_(sampleRate),
      audioEventHandlerRegistry_(audioEventHandlerRegistry),
      runtimeRegistry_(runtimeRegistry),
      audioEventScheduler_(AUDIO_SCHEDULER_CAPACITY),
      graphManager_(std::make_unique<AudioGraphManager>(this)),
      disposer_(
          std::make_unique<utils::DisposerImpl<utils::graph::Graph::kDisposerPayloadSize>>(
              AUDIO_SCHEDULER_CAPACITY)),
      graph_(std::make_shared<utils::graph::Graph>(AUDIO_SCHEDULER_CAPACITY, disposer_.get())) {}

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

std::shared_ptr<PeriodicWave> BaseAudioContext::createPeriodicWave(
    const std::vector<std::complex<float>> &complexData,
    bool disableNormalization,
    int length) const {
  return std::make_shared<PeriodicWave>(getSampleRate(), complexData, length, disableNormalization);
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

AudioGraphManager *BaseAudioContext::getGraphManager() const {
  return graphManager_.get();
}

std::shared_ptr<utils::graph::Graph> BaseAudioContext::getGraph() const {
  return graph_;
}

std::shared_ptr<IAudioEventHandlerRegistry> BaseAudioContext::getAudioEventHandlerRegistry() const {
  return audioEventHandlerRegistry_;
}

const RuntimeRegistry &BaseAudioContext::getRuntimeRegistry() const {
  return runtimeRegistry_;
}

utils::DisposerImpl<utils::graph::Graph::kDisposerPayloadSize> *BaseAudioContext::getDisposer()
    const {
  return disposer_.get();
}

} // namespace audioapi
