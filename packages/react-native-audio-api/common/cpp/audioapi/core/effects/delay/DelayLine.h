#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

namespace audioapi {

/// Shared delay ring + delay-time param.
///
/// Two logical read positions:
/// - `readIndex_` — advanced by DelayReader after each READ (live read head).
/// - `writeIndex_` — copy of `readIndex_` at the start of each audio quantum
///   (`syncReadSnapshotForQuantum` keyed by context sample frame). DelayWriter uses this to
///   compute write position so writer/reader order in the graph does not matter.
class DelayLine {
 public:
  DelayLine(std::shared_ptr<AudioBuffer> buffer, const std::shared_ptr<AudioParam> &delayTimeParam)
      : buffer_(std::move(buffer)), delayTimeParam_(delayTimeParam) {}

  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const {
    return buffer_;
  }

  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const {
    return delayTimeParam_;
  }

  /// Call at the start of DelayReader/DelayWriter `processNode` (same `currentSampleFrame` for
  /// the whole graph pass). First touch each quantum refreshes `readSnapshotForWrite_` from
  /// `readIndex_`.
  void syncReadSnapshotForQuantum(size_t currentSampleFrame) {
    if (currentSampleFrame != quantumSampleFrame_) {
      writeIndex_ = readIndex_;
      quantumSampleFrame_ = currentSampleFrame;
    }
  }

  /// Read head used by DelayWriter for `(snapshot + delaySamples) % N` (not `readIndex_` after
  /// reader may have advanced).
  [[nodiscard]] size_t readSnapshotForWrite() const {
    return writeIndex_;
  }

  [[nodiscard]] size_t &readIndex() {
    return readIndex_;
  }

  [[nodiscard]] size_t readIndex() const {
    return readIndex_;
  }

 private:
  std::shared_ptr<AudioBuffer> buffer_;
  std::shared_ptr<AudioParam> delayTimeParam_;

  size_t readIndex_{0};
  size_t writeIndex_{0};
  size_t quantumSampleFrame_{std::numeric_limits<size_t>::max()};
};

} // namespace audioapi
