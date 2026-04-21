#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/core/effects/delay/DelayReader.h>
#include <audioapi/core/effects/delay/DelayRingBufferOp.h>
#include <audioapi/core/effects/delay/DelayWriter.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

namespace audioapi {

DelayNode::DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options)
    : AudioNode(context, options),
      delayTimeParam_(
          std::make_shared<AudioParam>(options.delayTime, 0, options.maxDelayTime, context)),
      delayBuffer_(
          std::make_shared<AudioBuffer>(
              static_cast<size_t>(
                  options.maxDelayTime * context->getSampleRate() +
                  1), // +1 to enable delayTime equal to maxDelayTime
              channelCount_,
              context->getSampleRate())) {
  delayLine_ = std::make_shared<DelayLine>(delayBuffer_, delayTimeParam_);

  // Tail processing belongs to the writer: when the writer's upstream goes
  // silent, the writer must keep filling the ring with zeros for one full
  // `maxDelayTime` so that the reader naturally drains to silence. The
  // reader has no audio inputs (its data comes from the shared ring) and is
  // GC'd via the linkNodes(reader, writer) propagation, so it sets
  // `requiresTailProcessing = false` to opt out of the base-class state
  // machine that would otherwise consider it permanently "silent".
  AudioNodeOptions readerOptions = options;
  // Reader has no audio inputs (its data comes from the shared ring) and
  // does not own the tail — see comment above.
  readerOptions.numberOfInputs = 0;
  readerOptions.requiresTailProcessing = false;

  delayReader_ = std::make_unique<DelayReader>(context, readerOptions, delayLine_);
  delayWriter_ = std::make_unique<DelayWriter>(context, options, delayLine_);
}

std::shared_ptr<AudioParam> DelayNode::getDelayTimeParam() const {
  return delayTimeParam_;
}

} // namespace audioapi
