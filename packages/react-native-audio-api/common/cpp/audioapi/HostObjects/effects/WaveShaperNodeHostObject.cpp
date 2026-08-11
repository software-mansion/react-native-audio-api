#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/HostObjects/effects/WaveShaperNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/HostObjects/utils/NodeOptionsParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>

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

  if (options.curve != nullptr) {
    curveMemoryPressure_ = options.curve->getSize() * sizeof(float) * 2;
  }
}

void WaveShaperNodeHostObject::scheduleCurveUpdate(const std::shared_ptr<AudioArray> &curve) {
  auto handle = node_->handle;

  curveMemoryPressure_ = curve != nullptr ? curve->getSize() * sizeof(float) * 2 : 0;

  auto event = [handle, node = waveShaperNode_, curve](BaseAudioContext &) {
    node->setCurve(curve);
  };
  waveShaperNode_->scheduleAudioEvent(std::move(event));
}

JSI_HOST_FUNCTION_IMPL(WaveShaperNodeHostObject, setCurve) {
  std::shared_ptr<AudioArray> curve = nullptr;

  if (!args[0].isNull() && !args[0].isUndefined()) {
    curve = option_parser::parseCurveSequence(runtime, args[0]);
    if (curve != nullptr) {
      thisValue.asObject(runtime).setExternalMemoryPressure(
          runtime, getMemoryPressure() + curve->getSize() * sizeof(float) * 2);
    }
  }

  scheduleCurveUpdate(curve);
  return jsi::Value::undefined();
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

} // namespace audioapi
