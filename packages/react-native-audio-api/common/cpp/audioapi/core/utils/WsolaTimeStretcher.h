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

  void process(
      const DSPAudioBuffer &input,
      size_t inputFrames,
      DSPAudioBuffer &output,
      size_t outputFrames,
      float playbackRate);

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

  double outputTime_{0.0};
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
  float sampleAt(size_t channel, int frameIndex) const;
  void fillBlock(std::vector<std::vector<float>> &block, int frameIndex) const;
  void computeTargetEnergy();
  void updateOutputTime(float playbackRate, double timeChange);
  void removeOldInputFrames(float playbackRate);
  void compactInputQueue(size_t framesToRemove, float playbackRate);
};

} // namespace audioapi
