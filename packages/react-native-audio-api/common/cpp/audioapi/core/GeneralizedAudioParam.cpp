#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/GeneralizedAudioParam.h>
#include <audioapi/core/utils/Constants.h>

#include <memory>
#include <utility>

namespace audioapi {

GeneralizedAudioParam::GeneralizedAudioParam(
    float minValue,
    float maxValue,
    const std::shared_ptr<BaseAudioContext> &context)
    : context_(context),
      minValue_(minValue),
      maxValue_(maxValue),
      outputBuffer_(
          std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 1, context->getSampleRate())) {}

void GeneralizedAudioParam::finalizeARate(int framesToProcess, double time) {
  auto outputData = outputBuffer_->getChannel(0)->span();
  for (int i = 0; i < framesToProcess; ++i) {
    outputData[i] = clampToNominalRange(outputData[i]);
  }
  processedARateKey_ = std::make_pair(framesToProcess, time);
}

} // namespace audioapi
