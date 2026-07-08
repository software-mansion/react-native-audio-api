#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/effects/WaveShaperNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

WaveShaperNodeHostObject::WaveShaperNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const WaveShaperOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<WaveShaperNode>(context, options),
          options),
      waveShaperNode_(typedAudioNode<WaveShaperNode>(node_)),
      oversample_(options.oversample) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(WaveShaperNodeHostObject, oversample));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(WaveShaperNodeHostObject, oversample));
  addFunctions(JSI_EXPORT_FUNCTION(WaveShaperNodeHostObject, setCurve));
}

JSI_PROPERTY_GETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::overSampleTypeToString(oversample_));
}

JSI_PROPERTY_SETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  auto handle = node_->handle;
  auto oversample = js_enum_parser::overSampleTypeFromString(value.asString(runtime).utf8(runtime));

  // Build all resamplers on the JS thread so the audio thread only does
  // pointer swaps.
  const auto sampleRate = waveShaperNode_->getContextSampleRate();
  const auto channelCount = waveShaperNode_->getChannelCount();
  auto update = std::make_unique<OversampleUpdate>();
  update->type = oversample;
  update->pairs.reserve(channelCount);
  for (size_t i = 0; i < channelCount; ++i) {
    update->pairs.emplace_back(WaveShaper::makeResamplers(oversample, sampleRate));
  }

  auto event = [handle, node = waveShaperNode_, update = std::move(update)](
                   BaseAudioContext &context) mutable {
    node->setOversample(std::move(update), *context.getDisposer());
  };
  waveShaperNode_->scheduleAudioEvent(std::move(event));
  oversample_ = oversample;
}

JSI_HOST_FUNCTION_IMPL(WaveShaperNodeHostObject, setCurve) {
  auto handle = node_->handle;

  std::shared_ptr<AudioArray> curve = nullptr;

  if (args[0].isObject()) {
    auto arrayBuffer =
        args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    // *2 because the curve is copied into an internal AudioArray for processing.
    // Include the node's own base footprint; otherwise we'd clobber it.
    thisValue.asObject(runtime).setExternalMemoryPressure(
        runtime, getMemoryPressure() + arrayBuffer.size(runtime) * 2);

    auto size = static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float));
    curve =
        std::make_shared<AudioArray>(reinterpret_cast<float *>(arrayBuffer.data(runtime)), size);
  }

  auto event = [handle, node = waveShaperNode_, curve](BaseAudioContext &) {
    node->setCurve(curve);
  };
  waveShaperNode_->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
