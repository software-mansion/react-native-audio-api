#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/core/effects/delay/DelayReader.h>
#include <audioapi/core/effects/delay/DelayRingBufferOp.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

DelayReader::DelayReader(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioNodeOptions &options,
    std::shared_ptr<DelayLine> delayLine)
    : AudioNode(context, options), delayLine_(std::move(delayLine)) {}

void DelayReader::processNode(int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    audioBuffer_->zero();
    return;
  }

  delayLine_->syncReadSnapshotForQuantum(context->getCurrentSampleFrame());

  auto delayBuffer = delayLine_->getBuffer();
  size_t &readIdx = delayLine_->readIndex();

  delay_ring::bufferOperation(
      delayBuffer, audioBuffer_, framesToProcess, readIdx, delay_ring::BufferAction::READ);
}

} // namespace audioapi
