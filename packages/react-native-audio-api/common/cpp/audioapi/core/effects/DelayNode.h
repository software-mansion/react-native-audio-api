#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace audioapi {

struct DelayOptions;

class DelayNode : public AudioNode {
 public:
  explicit DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options);

  /// @note Any thread
  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const;

  /// @brief Number of frames the delay buffer holds.
  /// @note Audio Thread only
  [[nodiscard]] size_t getDelayBufferSize() const;

  /// A DelayNode may sit in a feedback loop. The spec then splits it into a
  /// DelayReader, a source that reads the delay buffer, and a DelayWriter, which
  /// writes the node's inputs into it. `processReader` is that reader and
  /// `processWriter` that writer. The reader runs first, which is the spec's
  /// minimum delay of one render quantum for a DelayNode in a cycle.
  /// https://webaudio.github.io/web-audio-api/#rendering-loop
  /// @note Any thread
  [[nodiscard]] bool canBreakCycle() const override {
    return true;
  }

  /// @brief A delay buffer as wide as `numberOfChannels`, sharing the samples
  /// of every channel the current one has.
  /// @note JS Thread only
  [[nodiscard]] std::shared_ptr<AudioBuffer> buildNegotiatedStorage(
      size_t numberOfChannels) override;

  /// @note Audio Thread only
  std::shared_ptr<AudioBuffer> exchangeNegotiatedStorage(
      std::shared_ptr<AudioBuffer> storage) override;

 protected:
  /// @note Audio Thread only
  void processNode(int framesToProcess) override;
  /// @note Audio Thread only
  void processReader(int framesToProcess) override;
  /// @note Audio Thread only
  void processWriter(const std::vector<const DSPAudioBuffer *> &inputs, int framesToProcess)
      override;

  /// @brief One full `maxDelayTime`: the longest a frame already in the delay
  /// buffer can wait before it is played.
  /// @note Audio Thread only
  [[nodiscard]] int computeTailFrames() const override;

 private:
  const std::shared_ptr<AudioParam> delayTimeParam_;
  /// Ring of the node's past inputs. The current quantum's input is stored at
  /// `writeIndex_`, and output frame `i` reads `delay(i)` frames behind
  /// `writeIndex_ + i`.
  std::shared_ptr<AudioBuffer> delayBuffer_;
  /// The delay buffer the latest negotiation built, which the audio thread may
  /// not have installed yet.
  /// @note JS Thread only
  std::shared_ptr<AudioBuffer> pendingDelayBuffer_;
  size_t writeIndex_ = 0;

  /// @brief Overwrites the current quantum's slots in the ring with the sum of
  /// `inputs`, mixed to the ring's channel layout.
  /// @note Audio Thread only
  void writeQuantum(std::span<const DSPAudioBuffer *const> inputs, int framesToProcess);

  /// @brief Renders the output from the ring, each frame `delayTime` at that
  /// frame behind the input it was written with, and at least `minDelayFrames`.
  /// @note Audio Thread only
  void readQuantum(int framesToProcess, double minDelayFrames);

  /// @note Audio Thread only
  void advanceWriteIndex(int framesToProcess);
};

} // namespace audioapi
