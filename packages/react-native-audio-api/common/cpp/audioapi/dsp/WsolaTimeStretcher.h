#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <vector>

namespace audioapi {

class WsolaTimeStretcher {
 public:
  WsolaTimeStretcher() = default;

  void configure(size_t channels, float sampleRate);
  void reset();

  [[nodiscard]] size_t getRequiredInputFrames() const {
    return searchIntervalFrames_ + windowSize_;
  }

  /// Minimum input frames needed for @ref canRunIteration at the current analysis
  /// pointers (search span + target window, accounting for pitchFactor_).
  [[nodiscard]] size_t getMinInputFramesToRun() const;

  [[nodiscard]] size_t getBufferedInputFrames() const {
    return inputQueue_.empty() ? 0 : inputQueue_[0].size();
  }

  [[nodiscard]] size_t getBufferedOutputFrames() const {
    return availableOutputFrames();
  }

  /// Appends PCM to the analysis queue without rendering output (startup prefill).
  void feedInput(const DSPAudioBuffer &input, size_t inputFrames);

  void process(
      const DSPAudioBuffer &input,
      size_t inputFrames,
      DSPAudioBuffer &output,
      size_t outputFrames,
      float playbackRate,
      float pitchFactor = 1.0f);

  /// @brief Renders already-buffered WSOLA output/input without appending new PCM.
  /// @return Number of frames written to @p output.
  size_t drainOutput(DSPAudioBuffer &output, size_t outputFrames, float playbackRate);

  // some arbitrary value has to be set here to limit the size of the buffer for wsola algorithm
  static constexpr float MAX_PLAYBACK_RATE = 4;

  /// Rough latency estimates for buffer tail padding (seconds).
  static constexpr float INPUT_LATENCY_MS = 20.0f;
  static constexpr float OUTPUT_LATENCY_MS = 10.0f;

  /// Scratch capacity for one cold-start warmup (~window+search) plus one max-rate quantum.
  [[nodiscard]] static size_t scratchBufferFrames(float sampleRate);

 private:
  static constexpr float OLA_WINDOW_MS = 20.0f;
  static constexpr float SEARCH_INTERVAL_MS = 30.0f;
  static constexpr size_t SEARCH_DECIMATION = 12;
  static constexpr size_t QUEUE_COMPACT_THRESHOLD_FRAMES = 4096;
  static constexpr size_t kFirstFramesToDump = 255;
  static constexpr float kDumpZeroThreshold = 1e-6f;

  size_t channels_{0};
  float sampleRate_{0.0f};
  size_t windowSize_{0};
  size_t hopSize_{0};
  size_t searchIntervalFrames_{0};
  size_t searchCenterOffset_{0};
  size_t maxInputFrames_{0};
  size_t excludeIntervalFrames_{0};

  float pitchFactor_{1.0f};

  double outputTime_{0.0};
  /// Cumulative synthesis-timeline position; advanced incrementally so rate changes do not
  /// rewrite history via outputTime_ * playbackRate.
  double synthesisPosition_{0.0};
  int targetBlockIndex_{0};
  int searchBlockIndex_{0};
  size_t outputReadIndex_{0};

  /// First OLA iteration after reset seeds @ref pendingOverlap_ with the leading
  /// block so output starts at full amplitude (no half-window fade-in).
  bool firstSynthesisIteration_{true};

  /// Persist across @ref drainOutput quanta so EOF silence is padded only once.
  bool drainEofSilencePadded_{false};
  bool drainPendingFlushed_{false};

  /// Startup-latency probe: first absolute non-zero output sample.
  bool firstSampleFound_{false};
  size_t totalFramesOutput_{0};

  /// One-shot dump of the first @c kFirstFramesToDump ch0 output samples after reset.
  bool firstFramesDumpPrinted_{false};
  std::vector<float> firstOutputFramesDump_;

  std::vector<float> olaWindow_;
  std::vector<float> transitionWindow_;
  std::vector<std::vector<float>> inputQueue_;
  std::vector<std::vector<float>> outputQueue_;
  std::vector<std::vector<float>> pendingOverlap_;
  std::vector<std::vector<float>> targetBlock_;
  std::vector<std::vector<float>> optimalBlock_;
  std::vector<float> targetEnergy_;
  // Contiguous, pitch-resampled copy of the current search region per channel.
  // Materialized once per findOptimalBlockIndex() so every candidate window is a
  // contiguous slice, enabling SIMD cross-correlation instead of per-sample
  // interpolation inside the search loop.
  std::vector<std::vector<float>> searchSpan_;

  void appendInput(const DSPAudioBuffer &input, size_t inputFrames);
  size_t availableOutputFrames() const;
  size_t writeOutput(DSPAudioBuffer &output, size_t outputOffset, size_t outputFrames);
  void compactOutputQueueIfNeeded();
  bool runOneIteration(float playbackRate);
  bool canRunIteration() const;
  bool targetIsWithinSearchRegion() const;
  int findOptimalBlockIndex();
  void fillSearchSpan();
  float similarityAt(int candidateIndex) const;
  [[nodiscard]] int maxSourceIndexForBlock(int blockStartFrame) const;
  float sampleAt(size_t channel, int frameIndex) const;
  void fillBlock(std::vector<std::vector<float>> &block, int frameIndex) const;
  void computeTargetEnergy();
  void updateOutputTime(float playbackRate, double timeChange);
  void removeOldInputFrames(float playbackRate);
  void compactInputQueue(size_t synthesisFramesToRemove, float playbackRate);
};

} // namespace audioapi
