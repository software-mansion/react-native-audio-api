#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/WsolaTimeStretcher.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace audioapi {
namespace {

size_t framesFromMs(float sampleRate, float ms) {
  return static_cast<size_t>(std::max(1.0f, std::round(sampleRate * ms / 1000.0f)));
}

float periodicHann(size_t n, size_t size) {
  return 0.5f * (1.0f - std::cos(2.0f * PI * static_cast<float>(n) / static_cast<float>(size)));
}

} // namespace

void WsolaTimeStretcher::configure(size_t channels, float sampleRate) {
  channels_ = channels;
  sampleRate_ = sampleRate > 0.0f ? sampleRate : DEFAULT_SAMPLE_RATE;

  windowSize_ = framesFromMs(sampleRate_, OLA_WINDOW_MS);
  windowSize_ += windowSize_ & 1U;
  hopSize_ = windowSize_ / 2;
  searchIntervalFrames_ = framesFromMs(sampleRate_, SEARCH_INTERVAL_MS);
  searchCenterOffset_ = searchIntervalFrames_ / 2 + (windowSize_ / 2 - 1);
  maxInputFrames_ = framesFromMs(sampleRate_, 500.0f);
  excludeIntervalFrames_ = framesFromMs(sampleRate_, 3.3f);

  olaWindow_.assign(windowSize_, 0.0f);
  for (size_t i = 0; i < windowSize_; ++i) {
    olaWindow_[i] = periodicHann(i, windowSize_);
  }

  transitionWindow_.assign(windowSize_ * 2, 0.0f);
  for (size_t i = 0; i < transitionWindow_.size(); ++i) {
    transitionWindow_[i] = periodicHann(i, transitionWindow_.size());
  }

  inputQueue_.assign(channels_, {});
  outputQueue_.assign(channels_, {});
  const size_t inputQueueCapacity =
      maxInputFrames_ + searchIntervalFrames_ + windowSize_ + RENDER_QUANTUM_SIZE * 4;
  const size_t outputQueueCapacity = hopSize_ * 4 + RENDER_QUANTUM_SIZE * 4;
  for (auto &channel : inputQueue_) {
    channel.reserve(inputQueueCapacity);
  }
  for (auto &channel : outputQueue_) {
    channel.reserve(outputQueueCapacity);
  }
  pendingOverlap_.assign(channels_, std::vector<float>(hopSize_, 0.0f));
  targetBlock_.assign(channels_, std::vector<float>(windowSize_, 0.0f));
  optimalBlock_.assign(channels_, std::vector<float>(windowSize_, 0.0f));
  targetEnergy_.assign(channels_, 0.0f);

  reset();
}

void WsolaTimeStretcher::reset() {
  outputTime_ = 0.0;
  targetBlockIndex_ = 0;
  searchBlockIndex_ = 0;
  outputReadIndex_ = 0;

  for (auto &channel : inputQueue_) {
    channel.clear();
  }
  for (auto &channel : outputQueue_) {
    channel.clear();
  }
  for (auto &channel : pendingOverlap_) {
    std::fill(channel.begin(), channel.end(), 0.0f);
  }
}

void WsolaTimeStretcher::process(
    const DSPAudioBuffer &input,
    size_t inputFrames,
    DSPAudioBuffer &output,
    size_t outputFrames,
    float playbackRate) {
  output.zero();

  if (channels_ == 0 || windowSize_ == 0 || playbackRate <= 0.0f || outputFrames == 0) {
    return;
  }

  appendInput(input, inputFrames);

  size_t rendered = 0;
  while (rendered < outputFrames) {
    rendered += writeOutput(output, rendered, outputFrames - rendered);
    if (rendered >= outputFrames) {
      break;
    }
    if (!runOneIteration(playbackRate)) {
      break;
    }
  }

  if (playbackRate < 1.0f && rendered >= outputFrames &&
      availableOutputFrames() < outputFrames * 2) {
    runOneIteration(playbackRate);
  }
}

void WsolaTimeStretcher::appendInput(const DSPAudioBuffer &input, size_t inputFrames) {
  const size_t frames = std::min(inputFrames, input.getSize());
  if (frames == 0) {
    return;
  }

  for (size_t channel = 0; channel < channels_; ++channel) {
    const float *source = input.getChannel(channel)->begin();
    auto &queue = inputQueue_[channel];
    queue.insert(queue.end(), source, source + frames);
  }
}

size_t WsolaTimeStretcher::availableOutputFrames() const {
  if (outputQueue_.empty() || outputQueue_[0].size() <= outputReadIndex_) {
    return 0;
  }

  return outputQueue_[0].size() - outputReadIndex_;
}

size_t
WsolaTimeStretcher::writeOutput(DSPAudioBuffer &output, size_t outputOffset, size_t outputFrames) {
  if (outputFrames == 0 || outputQueue_.empty()) {
    return 0;
  }

  const size_t availableFrames = availableOutputFrames();
  if (availableFrames == 0) {
    compactOutputQueueIfNeeded();
    return 0;
  }

  const size_t frames = std::min(outputFrames, availableFrames);
  for (size_t channel = 0; channel < channels_; ++channel) {
    auto &queue = outputQueue_[channel];
    float *destination = output.getChannel(channel)->begin() + outputOffset;
    std::copy_n(queue.begin() + static_cast<std::ptrdiff_t>(outputReadIndex_), frames, destination);
  }
  outputReadIndex_ += frames;
  compactOutputQueueIfNeeded();

  return frames;
}

void WsolaTimeStretcher::compactOutputQueueIfNeeded() {
  if (outputQueue_.empty() || outputReadIndex_ == 0) {
    return;
  }

  const size_t queueSize = outputQueue_[0].size();
  if (outputReadIndex_ >= queueSize) {
    for (auto &queue : outputQueue_) {
      queue.clear();
    }
    outputReadIndex_ = 0;
    return;
  }

  const size_t remainingFrames = queueSize - outputReadIndex_;
  if (outputReadIndex_ < QUEUE_COMPACT_THRESHOLD_FRAMES && outputReadIndex_ < remainingFrames) {
    return;
  }

  for (auto &queue : outputQueue_) {
    queue.erase(queue.begin(), queue.begin() + static_cast<std::ptrdiff_t>(outputReadIndex_));
  }
  outputReadIndex_ = 0;
}

bool WsolaTimeStretcher::runOneIteration(float playbackRate) {
  if (!canRunIteration()) {
    return false;
  }

  const bool transitionNeeded = !targetIsWithinSearchRegion();
  const int optimalIndex = transitionNeeded ? findOptimalBlockIndex() : targetBlockIndex_;

  fillBlock(optimalBlock_, optimalIndex);

  if (transitionNeeded) {
    fillBlock(targetBlock_, targetBlockIndex_);
    for (size_t channel = 0; channel < channels_; ++channel) {
      for (size_t frame = 0; frame < windowSize_; ++frame) {
        optimalBlock_[channel][frame] = optimalBlock_[channel][frame] * transitionWindow_[frame] +
            targetBlock_[channel][frame] * transitionWindow_[windowSize_ + frame];
      }
    }
  }

  for (size_t channel = 0; channel < channels_; ++channel) {
    auto &output = outputQueue_[channel];
    compactOutputQueueIfNeeded();
    output.reserve(output.size() + hopSize_);

    for (size_t frame = 0; frame < hopSize_; ++frame) {
      output.push_back(
          pendingOverlap_[channel][frame] * olaWindow_[hopSize_ + frame] +
          optimalBlock_[channel][frame] * olaWindow_[frame]);
    }

    for (size_t frame = 0; frame < hopSize_; ++frame) {
      pendingOverlap_[channel][frame] = optimalBlock_[channel][hopSize_ + frame];
    }
  }

  targetBlockIndex_ = optimalIndex + static_cast<int>(hopSize_);
  updateOutputTime(playbackRate, static_cast<double>(hopSize_));
  removeOldInputFrames(playbackRate);

  return true;
}

bool WsolaTimeStretcher::canRunIteration() const {
  if (inputQueue_.empty()) {
    return false;
  }

  const int inputFrames = static_cast<int>(inputQueue_[0].size());
  const int searchBlockSize = static_cast<int>(searchIntervalFrames_ + windowSize_ - 1);

  if (targetBlockIndex_ + static_cast<int>(windowSize_) > inputFrames) {
    return false;
  }

  if (targetIsWithinSearchRegion()) {
    return true;
  }

  return searchBlockIndex_ + searchBlockSize <= inputFrames;
}

bool WsolaTimeStretcher::targetIsWithinSearchRegion() const {
  const int searchBlockSize = static_cast<int>(searchIntervalFrames_ + windowSize_ - 1);
  return targetBlockIndex_ >= searchBlockIndex_ &&
      targetBlockIndex_ + static_cast<int>(windowSize_) <= searchBlockIndex_ + searchBlockSize;
}

int WsolaTimeStretcher::findOptimalBlockIndex() {
  fillBlock(targetBlock_, targetBlockIndex_);
  computeTargetEnergy();

  const int candidateCount = static_cast<int>(searchIntervalFrames_);
  if (candidateCount <= 1) {
    return searchBlockIndex_;
  }

  const int previousOptimal = targetBlockIndex_ - static_cast<int>(hopSize_);
  const int excludeHalf = static_cast<int>(excludeIntervalFrames_ / 2);
  const int excludeStart = previousOptimal - excludeHalf;
  const int excludeEnd = previousOptimal + excludeHalf;

  auto isExcluded = [&](int index) {
    return index >= excludeStart && index <= excludeEnd;
  };

  float bestScore = -std::numeric_limits<float>::infinity();
  int bestIndex = searchBlockIndex_;

  for (int offset = 0; offset < candidateCount; offset += static_cast<int>(SEARCH_DECIMATION)) {
    const int index = searchBlockIndex_ + offset;
    if (isExcluded(index)) {
      continue;
    }

    const float score = similarityAt(index);
    if (score > bestScore) {
      bestScore = score;
      bestIndex = index;
    }
  }

  const int refineStart =
      std::max(searchBlockIndex_, bestIndex - static_cast<int>(SEARCH_DECIMATION));
  const int refineEnd = std::min(
      searchBlockIndex_ + candidateCount - 1, bestIndex + static_cast<int>(SEARCH_DECIMATION));

  for (int index = refineStart; index <= refineEnd; ++index) {
    if (isExcluded(index)) {
      continue;
    }

    const float score = similarityAt(index);
    if (score > bestScore) {
      bestScore = score;
      bestIndex = index;
    }
  }

  return bestIndex;
}

float WsolaTimeStretcher::similarityAt(int candidateIndex) const {
  static constexpr float EPSILON = 1e-12f;
  float score = 0.0f;

  for (size_t channel = 0; channel < channels_; ++channel) {
    float dot = 0.0f;
    float candidateEnergy = 0.0f;

    for (size_t frame = 0; frame < windowSize_; frame += SIMILARITY_FRAME_STRIDE) {
      const float target = targetBlock_[channel][frame];
      const float candidate = sampleAt(channel, candidateIndex + static_cast<int>(frame));
      dot += target * candidate;
      candidateEnergy += candidate * candidate;
    }

    score += dot / std::sqrt(targetEnergy_[channel] * candidateEnergy + EPSILON);
  }

  return score;
}

float WsolaTimeStretcher::sampleAt(size_t channel, int frameIndex) const {
  if (frameIndex < 0 || channel >= inputQueue_.size()) {
    return 0.0f;
  }

  const auto &queue = inputQueue_[channel];
  const auto index = static_cast<size_t>(frameIndex);
  return index < queue.size() ? queue[index] : 0.0f;
}

void WsolaTimeStretcher::fillBlock(std::vector<std::vector<float>> &block, int frameIndex) const {
  for (size_t channel = 0; channel < channels_; ++channel) {
    for (size_t frame = 0; frame < windowSize_; ++frame) {
      block[channel][frame] = sampleAt(channel, frameIndex + static_cast<int>(frame));
    }
  }
}

void WsolaTimeStretcher::computeTargetEnergy() {
  for (size_t channel = 0; channel < channels_; ++channel) {
    float energy = 0.0f;
    for (size_t frame = 0; frame < windowSize_; frame += SIMILARITY_FRAME_STRIDE) {
      const float value = targetBlock_[channel][frame];
      energy += value * value;
    }
    targetEnergy_[channel] = energy;
  }
}

void WsolaTimeStretcher::updateOutputTime(float playbackRate, double timeChange) {
  outputTime_ += timeChange;
  const int searchBlockCenterIndex =
      static_cast<int>(outputTime_ * static_cast<double>(playbackRate) + 0.5);
  searchBlockIndex_ = searchBlockCenterIndex - static_cast<int>(searchCenterOffset_);
}

void WsolaTimeStretcher::removeOldInputFrames(float playbackRate) {
  const int earliestUsedIndex = std::min(targetBlockIndex_, searchBlockIndex_);
  if (earliestUsedIndex <= 0) {
    return;
  }

  const auto framesToRemove = static_cast<size_t>(earliestUsedIndex);
  if (framesToRemove < QUEUE_COMPACT_THRESHOLD_FRAMES && inputQueue_[0].size() <= maxInputFrames_) {
    return;
  }

  compactInputQueue(framesToRemove, playbackRate);
}

void WsolaTimeStretcher::compactInputQueue(size_t framesToRemove, float playbackRate) {
  if (inputQueue_.empty() || framesToRemove == 0) {
    return;
  }

  framesToRemove = std::min(framesToRemove, inputQueue_[0].size());
  for (auto &queue : inputQueue_) {
    if (framesToRemove >= queue.size()) {
      queue.clear();
    } else {
      queue.erase(queue.begin(), queue.begin() + static_cast<std::ptrdiff_t>(framesToRemove));
    }
  }

  const int removedFrames = static_cast<int>(framesToRemove);
  targetBlockIndex_ -= removedFrames;
  searchBlockIndex_ -= removedFrames;
  outputTime_ = std::max(0.0, outputTime_ - static_cast<double>(removedFrames) / playbackRate);
}

} // namespace audioapi
