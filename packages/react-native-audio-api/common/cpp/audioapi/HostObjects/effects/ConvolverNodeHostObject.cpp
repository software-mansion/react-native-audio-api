#include <audioapi/HostObjects/effects/ConvolverNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <audioapi/utils/ThreadPool.hpp>

#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

ConvolverNodeHostObject::ConvolverNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConvolverOptions &options)
    : AudioNodeHostObject(context->createConvolver(options), options),
      normalize_(!options.disableNormalization) {
  if (options.buffer != nullptr) {
    setBuffer(options.buffer);
  }

  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, normalize));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, normalize));
  addFunctions(JSI_EXPORT_FUNCTION(ConvolverNodeHostObject, setBuffer));
}

JSI_PROPERTY_GETTER_IMPL(ConvolverNodeHostObject, normalize) {
  return jsi::Value(normalize_);
}

JSI_PROPERTY_SETTER_IMPL(ConvolverNodeHostObject, normalize) {
  normalize_ = value.getBool();
  ;
}

JSI_HOST_FUNCTION_IMPL(ConvolverNodeHostObject, setBuffer) {
  if (!args[0].isObject()) {
    return jsi::Value::undefined();
  }

  auto bufferHostObject = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  thisValue.asObject(runtime).setExternalMemoryPressure(
      runtime, bufferHostObject->getSizeInBytes());

  setBuffer(bufferHostObject->audioBuffer_);

  return jsi::Value::undefined();
}

void ConvolverNodeHostObject::setBuffer(const std::shared_ptr<AudioBuffer> &buffer) {
  if (buffer == nullptr) {
    return;
  }

  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);

  auto copiedBuffer = std::make_shared<AudioBuffer>(*buffer);

  float scaleFactor = 1.0f;
  if (normalize_) {
    scaleFactor = convolverNode->calculateNormalizationScale(copiedBuffer);
  }

  auto threadPool = std::make_shared<ThreadPool>(4);
  std::vector<Convolver> convolvers;
  for (size_t i = 0; i < copiedBuffer->getNumberOfChannels(); ++i) {
    AudioArray channelData(*copiedBuffer->getChannel(i));
    convolvers.emplace_back();
    convolvers.back().init(RENDER_QUANTUM_SIZE, channelData, copiedBuffer->getSize());
  }
  if (copiedBuffer->getNumberOfChannels() == 1) {
    // add one more convolver, because right now input is always stereo
    AudioArray channelData(*copiedBuffer->getChannel(0));
    convolvers.emplace_back();
    convolvers.back().init(RENDER_QUANTUM_SIZE, channelData, copiedBuffer->getSize());
  }

  auto internalBuffer = std::make_shared<AudioBuffer>(
      RENDER_QUANTUM_SIZE * 2, convolverNode->getChannelCount(), copiedBuffer->getSampleRate());
  auto intermediateBuffer = std::make_shared<AudioBuffer>(
      RENDER_QUANTUM_SIZE, convolvers.size(), copiedBuffer->getSampleRate());

  struct SetupData {
    std::shared_ptr<AudioBuffer> buffer;
    std::vector<Convolver> convolvers;
    std::shared_ptr<ThreadPool> threadPool;
    std::shared_ptr<AudioBuffer> internalBuffer;
    std::shared_ptr<AudioBuffer> intermediateBuffer;
    float scaleFactor;
  };

  auto setupData = std::make_shared<SetupData>(SetupData{
      copiedBuffer,
      std::move(convolvers),
      threadPool,
      internalBuffer,
      intermediateBuffer,
      scaleFactor});

  auto event = [convolverNode, setupData](BaseAudioContext &) {
    convolverNode->setBuffer(
        setupData->buffer,
        std::move(setupData->convolvers),
        setupData->threadPool,
        setupData->internalBuffer,
        setupData->intermediateBuffer,
        setupData->scaleFactor);
  };
  convolverNode->scheduleAudioEvent(std::move(event));
}
} // namespace audioapi
