#include <audioapi/HostObjects/effects/WaveShaperNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <memory>
#include <utility>

namespace audioapi {

WaveShaperNodeHostObject::WaveShaperNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const WaveShaperOptions &options)
    : AudioNodeHostObject(context->createWaveShaper(options), options),
      oversample_(options.oversample) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(WaveShaperNodeHostObject, oversample));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(WaveShaperNodeHostObject, oversample));
  addFunctions(JSI_EXPORT_FUNCTION(WaveShaperNodeHostObject, setCurve));
}

JSI_PROPERTY_GETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::overSampleTypeToString(oversample_));
}

JSI_PROPERTY_SETTER_IMPL(WaveShaperNodeHostObject, oversample) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);

  auto oversample = js_enum_parser::overSampleTypeFromString(value.asString(runtime).utf8(runtime));
  auto event = [waveShaperNode, oversample](BaseAudioContext &) {
    waveShaperNode->setOversample(oversample);
  };
  waveShaperNode->scheduleAudioEvent(std::move(event));
  oversample_ = oversample;
}

JSI_HOST_FUNCTION_IMPL(WaveShaperNodeHostObject, setCurve) {
  auto waveShaperNode = std::static_pointer_cast<WaveShaperNode>(node_);

  std::shared_ptr<AudioArray> curve = nullptr;

  if (args[0].isObject()) {
    auto arrayBuffer =
        args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    // *2 because it is copied to internal curve array for processing
    thisValue.asObject(runtime).setExternalMemoryPressure(runtime, arrayBuffer.size(runtime) * 2);

    auto size = static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float));
    curve = std::make_shared<AudioArrayBuffer>(
        reinterpret_cast<float *>(arrayBuffer.data(runtime)), size);
  }

  auto event = [waveShaperNode, curve](BaseAudioContext &) {
    waveShaperNode->setCurve(curve);
  };
  waveShaperNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
