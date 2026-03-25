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
  auto waveShaperNode = static_cast<WaveShaperNode *>(handle->audioNode->asAudioNode());

  auto oversample = js_enum_parser::overSampleTypeFromString(value.asString(runtime).utf8(runtime));
  auto event = [handle, oversample](BaseAudioContext &) {
    static_cast<WaveShaperNode *>(handle->audioNode->asAudioNode())->setOversample(oversample);
  };
  waveShaperNode->scheduleAudioEvent(std::move(event));
  oversample_ = oversample;
}

JSI_HOST_FUNCTION_IMPL(WaveShaperNodeHostObject, setCurve) {
  auto handle = node_->handle;
  auto waveShaperNode = static_cast<WaveShaperNode *>(handle->audioNode->asAudioNode());

  std::shared_ptr<AudioArray> curve = nullptr;

  if (args[0].isObject()) {
    auto arrayBuffer =
        args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    // *2 because it is copied to internal curve array for processing
    thisValue.asObject(runtime).setExternalMemoryPressure(runtime, arrayBuffer.size(runtime) * 2);

    auto size = static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float));
    curve =
        std::make_shared<AudioArray>(reinterpret_cast<float *>(arrayBuffer.data(runtime)), size);
  }

  auto event = [handle, curve](BaseAudioContext &) {
    static_cast<WaveShaperNode *>(handle->audioNode->asAudioNode())->setCurve(curve);
  };
  waveShaperNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}

} // namespace audioapi
