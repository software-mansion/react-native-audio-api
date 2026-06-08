#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/core/effects/delay/DelayRingBufferOp.h>
#include <audioapi/core/effects/delay/DelayWriter.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

DelayWriter::DelayWriter(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioNodeOptions &options,
    std::shared_ptr<DelayLine> delayLine)
    : AudioNode(context, options), delayLine_(std::move(delayLine)) {}

void DelayWriter::processNode(int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  delayLine_->syncReadSnapshotForQuantum(context->getCurrentSampleFrame());

  auto delayBuffer = delayLine_->getBuffer();
  auto delayTime = delayLine_->getDelayTimeParam()->processKRateParam(
      framesToProcess, context->getCurrentTime());
  const size_t readForWrite = delayLine_->readSnapshotForWrite();
  auto writeIndex =
      static_cast<size_t>(static_cast<float>(readForWrite) + delayTime * context->getSampleRate()) %
      delayBuffer->getSize();

  delay_ring::bufferOperation(
      delayBuffer, audioBuffer_, framesToProcess, writeIndex, delay_ring::BufferAction::WRITE);
}

int DelayWriter::computeTailFrames() const {
  // `getMaxValue()` of the delayTime param was set to `maxDelayTime` by the
  // owning DelayNode constructor.
  const auto sampleRate = delayLine_->getBuffer() ? delayLine_->getBuffer()->getSampleRate() : 0.0f;
  const float maxDelaySeconds = delayLine_->getDelayTimeParam()->getMaxValue();
  return static_cast<int>(maxDelaySeconds * sampleRate);
}

} // namespace audioapi
