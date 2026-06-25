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

  [[nodiscard]] size_t getBufferedInputFrames() const {
    return inputQueue_.empty() ? 0 : inputQueue_[0].size();
  }

  [[nodiscard]] size_t getBufferedOutputFrames() const {
    return availableOutputFrames();
  }

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

 private:
  static constexpr float OLA_WINDOW_MS = 20.0f;
  static constexpr float SEARCH_INTERVAL_MS = 30.0f;
  // Chromium uses a denser decimated search, but pairs it with SIMD dot products
  // and precomputed moving energies. This lighter search keeps iOS route changes
  // from overloading the real-time audio callback in the current implementation.
  static constexpr size_t SEARCH_DECIMATION = 12;
  static constexpr size_t SIMILARITY_FRAME_STRIDE = 4;
  static constexpr size_t QUEUE_COMPACT_THRESHOLD_FRAMES = 4096;

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

  std::vector<float> olaWindow_;
  std::vector<float> transitionWindow_;
  std::vector<std::vector<float>> inputQueue_;
  std::vector<std::vector<float>> outputQueue_;
  std::vector<std::vector<float>> pendingOverlap_;
  std::vector<std::vector<float>> targetBlock_;
  std::vector<std::vector<float>> optimalBlock_;
  std::vector<float> targetEnergy_;

  void appendInput(const DSPAudioBuffer &input, size_t inputFrames);
  size_t availableOutputFrames() const;
  size_t writeOutput(DSPAudioBuffer &output, size_t outputOffset, size_t outputFrames);
  void compactOutputQueueIfNeeded();
  bool runOneIteration(float playbackRate);
  bool canRunIteration() const;
  bool targetIsWithinSearchRegion() const;
  int findOptimalBlockIndex();
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
