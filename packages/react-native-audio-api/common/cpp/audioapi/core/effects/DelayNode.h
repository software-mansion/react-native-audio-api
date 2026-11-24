#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace audioapi {

struct DelayOptions;

class DelayNode : public AudioNode {
 public:
  explicit DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const;

  /// @brief Number of frames the delay buffer holds.
  [[nodiscard]] size_t getDelayBufferSize() const;

  /// A DelayNode may sit in a feedback loop. The spec then splits it into a
  /// DelayReader, a source that reads the delay buffer, and a DelayWriter, which
  /// writes the node's inputs into it. `processReader` is that reader and
  /// `processWriter` that writer. The reader runs first, which is the spec's
  /// minimum delay of one render quantum for a DelayNode in a cycle.
  /// https://webaudio.github.io/web-audio-api/#rendering-loop
  [[nodiscard]] bool canBreakCycle() const override {
    return true;
  }

 protected:
  void processNode(int framesToProcess) override;
  void processReader(int framesToProcess) override;
  void processWriter(const std::vector<const DSPAudioBuffer *> &inputs, int framesToProcess)
      override;

  /// @brief One full `maxDelayTime`: the longest a frame already in the delay
  /// buffer can wait before it is played.
  [[nodiscard]] int computeTailFrames() const override;

 private:
  const std::shared_ptr<AudioParam> delayTimeParam_;
  /// Ring of frames waiting to be played. The frame at `readIndex_` is the next
  /// one to output, so a frame stored `d` frames past it comes out `d` frames later.
  std::shared_ptr<AudioBuffer> delayBuffer_;
  size_t readIndex_ = 0;

  /// @brief The delay for the quantum being rendered, in frames.
  [[nodiscard]] size_t currentDelayFrames() const;

  /// @brief Writes `frames` frames of `source` into the ring, starting at
  /// `ringIndex`, on top of what other inputs already wrote there.
  void writeToDelayBuffer(const DSPAudioBuffer &source, size_t ringIndex, int frames);

  /// @brief Reads the next `frames` frames of the ring into the output buffer
  /// and advances the read head past them.
  void readFromDelayBuffer(int frames);
};

} // namespace audioapi
