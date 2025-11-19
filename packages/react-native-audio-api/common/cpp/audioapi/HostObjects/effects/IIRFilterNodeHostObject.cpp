#include <audioapi/HostObjects/effects/IIRFilterNodeHostObject.h>
#include <audioapi/core/effects/IIRFilterNode.h>
#include <memory>

namespace audioapi {

IIRFilterNodeHostObject::IIRFilterNodeHostObject(const std::shared_ptr<IIRFilterNode> &node)
    : AudioNodeHostObject(node) {

  addFunctions(JSI_EXPORT_FUNCTION(IIRFilterNodeHostObject, getFrequencyResponse));
}

JSI_HOST_FUNCTION_IMPL(IIRFilterNodeHostObject, getFrequencyResponse) {
  auto arrayBufferFrequency =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto frequencyArray = reinterpret_cast<float *>(arrayBufferFrequency.data(runtime));
  auto length = static_cast<size_t>(arrayBufferFrequency.size(runtime));

  auto arrayBufferMag =
      args[1].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto magResponseOut = reinterpret_cast<float *>(arrayBufferMag.data(runtime));

  auto arrayBufferPhase =
      args[2].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto phaseResponseOut = reinterpret_cast<float *>(arrayBufferPhase.data(runtime));

  auto iirFilterNode = std::static_pointer_cast<IIRFilterNode>(node_);
  iirFilterNode->getFrequencyResponse(frequencyArray, magResponseOut, phaseResponseOut, length);

  return jsi::Value::undefined();
}

} // namespace audioapi
