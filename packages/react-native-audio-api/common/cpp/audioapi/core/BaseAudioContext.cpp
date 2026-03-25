#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/core/utils/graph/DestinationGraphObject.hpp>
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
      disposer_(
          std::make_unique<utils::DisposerImpl<utils::graph::Graph::kDisposerPayloadSize>>(
              AUDIO_SCHEDULER_CAPACITY)),
      graph_(std::make_shared<utils::graph::Graph>(AUDIO_SCHEDULER_CAPACITY, disposer_.get())) {}

utils::graph::HostGraph::Node *BaseAudioContext::initialize() {
  destination_ = std::make_shared<AudioDestinationNode>(shared_from_this());
  return graph_->addNode(std::make_unique<DestinationGraphObject>(destination_.get()));
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
  return currentSampleFrame_.load(std::memory_order_acquire);
}

double BaseAudioContext::getCurrentTime() const {
  return static_cast<double>(getCurrentSampleFrame()) / getSampleRate();
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

void BaseAudioContext::processGraph(DSPAudioBuffer *buffer, int numFrames) {
  graph_->processEvents();
  graph_->process();
  processAudioEvents();

  for (auto &&[node, inputs] : graph_->iter()) {
    auto audioNode = node.asAudioNode();
    if (audioNode != nullptr) {
      audioNode->process(inputs, numFrames);
      if (audioNode == destination_.get()) {
        buffer->copy(*audioNode->getAudioBuffer(), 0, 0, numFrames);
      }
    }
  }

  currentSampleFrame_.fetch_add(numFrames, std::memory_order_release);
}

} // namespace audioapi
