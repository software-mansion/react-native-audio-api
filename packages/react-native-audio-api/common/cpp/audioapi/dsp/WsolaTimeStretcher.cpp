#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
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

size_t WsolaTimeStretcher::scratchBufferFrames(float sampleRate) {
  const float sr = sampleRate > 0.0f ? sampleRate : DEFAULT_SAMPLE_RATE;
  // Match configure(): window (+1 if odd) + search, plus one max-rate quantum.
  size_t window = framesFromMs(sr, OLA_WINDOW_MS);
  window += window & 1U;
  return window + framesFromMs(sr, SEARCH_INTERVAL_MS) +
      static_cast<size_t>(MAX_PLAYBACK_RATE * RENDER_QUANTUM_SIZE);
}

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
  const size_t inputQueueCapacity = maxInputFrames_ + searchIntervalFrames_ + windowSize_ +
      static_cast<size_t>(RENDER_QUANTUM_SIZE * 4);
  const size_t outputQueueCapacity = hopSize_ * 4 + static_cast<size_t>(RENDER_QUANTUM_SIZE * 4);
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
  // Search span covers every candidate offset [0, searchIntervalFrames_) plus a
  // full trailing window, so any candidate slice stays in bounds.
  searchSpan_.assign(channels_, std::vector<float>(searchIntervalFrames_ + windowSize_, 0.0f));

  reset();
}

void WsolaTimeStretcher::reset() {
  outputTime_ = 0.0;
  synthesisPosition_ = 0.0;
  targetBlockIndex_ = 0;
  searchBlockIndex_ = 0;
  outputReadIndex_ = 0;

  firstSynthesisIteration_ = true;
  drainEofSilencePadded_ = false;
  drainPendingFlushed_ = false;

  firstSampleFound_ = false;
  firstRelativeSampleFound_ = false;
  outputPeakAbs_ = 0.0f;
  totalFramesOutput_ = 0;

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

size_t WsolaTimeStretcher::getMinInputFramesToRun() const {
  if (windowSize_ == 0 || searchIntervalFrames_ == 0) {
    return 0;
  }

  // Last search candidate starts at searchBlockIndex_ + searchIntervalFrames_ - 1.
  const int lastCandidateStart = searchBlockIndex_ + static_cast<int>(searchIntervalFrames_) - 1;
  const int targetNeed = maxSourceIndexForBlock(targetBlockIndex_);
  const int searchNeed = maxSourceIndexForBlock(lastCandidateStart);
  return static_cast<size_t>(std::max(targetNeed, searchNeed) + 1);
}

void WsolaTimeStretcher::feedInput(const DSPAudioBuffer &input, size_t inputFrames) {
  if (channels_ == 0 || windowSize_ == 0 || inputFrames == 0) {
    return;
  }
  appendInput(input, inputFrames);
}

void WsolaTimeStretcher::process(
    const DSPAudioBuffer &input,
    size_t inputFrames,
    DSPAudioBuffer &output,
    size_t outputFrames,
    float playbackRate,
    float pitchFactor) {
  output.zero();

  pitchFactor_ = pitchFactor > 0.0f ? pitchFactor : 1.0f;

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

  if (!firstSampleFound_ || !firstRelativeSampleFound_) {
    // Absolute floor catches any non-zero; relative waits for a meaningful peak so
    // quiet leading content does not look like algorithmic latency.
    constexpr float kAbsoluteThreshold = 1e-6f;
    constexpr float kRelativeFraction = 0.5f;
    constexpr float kMinPeakForRelative = 0.05f;

    for (size_t i = 0; i < output.getNumberOfChannels(); ++i) {
      auto *channel = output.getChannel(i);
      for (size_t j = 0; j < channel->getSize(); ++j) {
        const float absSample = std::abs((*channel)[j]);
        outputPeakAbs_ = std::max(outputPeakAbs_, absSample);

        if (!firstSampleFound_ && absSample > kAbsoluteThreshold) {
          firstSampleFound_ = true;
          const size_t absoluteFrameIndex = totalFramesOutput_ + j;
          const double latencyMs = (static_cast<double>(absoluteFrameIndex) / sampleRate_) * 1000.0;
          std::cout << "[WSOLA] Pierwszy dzwiek (abs>1e-6)! "
                    << "Ramka wyjsciowa: " << absoluteFrameIndex << " | Opoznienie: " << latencyMs
                    << " ms"
                    << " | Playback Rate: " << playbackRate << std::endl;
        }

        if (!firstRelativeSampleFound_ && outputPeakAbs_ >= kMinPeakForRelative &&
            absSample >= kRelativeFraction * outputPeakAbs_) {
          firstRelativeSampleFound_ = true;
          const size_t absoluteFrameIndex = totalFramesOutput_ + j;
          const double latencyMs = (static_cast<double>(absoluteFrameIndex) / sampleRate_) * 1000.0;
          std::cout << "[WSOLA] Pierwszy dzwiek (rel>=" << kRelativeFraction
                    << "*peak, peak>=" << kMinPeakForRelative << ")! "
                    << "Ramka wyjsciowa: " << absoluteFrameIndex << " | Opoznienie: " << latencyMs
                    << " ms"
                    << " | peak=" << outputPeakAbs_ << " | Playback Rate: " << playbackRate
                    << std::endl;
        }

        if (firstSampleFound_ && firstRelativeSampleFound_) {
          break;
        }
      }
      if (firstSampleFound_ && firstRelativeSampleFound_) {
        break;
      }
    }
  }

  totalFramesOutput_ += outputFrames;
}

size_t
WsolaTimeStretcher::drainOutput(DSPAudioBuffer &output, size_t outputFrames, float playbackRate) {
  if (channels_ == 0 || windowSize_ == 0 || playbackRate <= 0.0f || outputFrames == 0) {
    return 0;
  }

  size_t rendered = 0;
  while (rendered < outputFrames) {
    rendered += writeOutput(output, rendered, outputFrames - rendered);
    if (rendered >= outputFrames) {
      break;
    }
    if (runOneIteration(playbackRate)) {
      continue;
    }

    // Pad one analysis window of silence so the last partial search/target region
    // can still form hops. Once per drain session (not per quantum) — otherwise
    // each quantum re-pads and playback never ends.
    if (!drainEofSilencePadded_ && hopSize_ > 0 && !inputQueue_.empty()) {
      drainEofSilencePadded_ = true;
      const size_t pad = searchIntervalFrames_ + windowSize_;
      bool canPad = true;
      for (const auto &channel : inputQueue_) {
        if (channel.size() + pad > channel.capacity()) {
          canPad = false;
          break;
        }
      }
      if (canPad) {
        for (auto &channel : inputQueue_) {
          channel.insert(channel.end(), pad, 0.0f);
        }
        continue;
      }
    }

    // Flush leftover half-window once so EOF does not discard pendingOverlap_.
    if (!drainPendingFlushed_ && hopSize_ > 0) {
      drainPendingFlushed_ = true;
      for (size_t channel = 0; channel < channels_; ++channel) {
        auto &outQueue = outputQueue_[channel];
        compactOutputQueueIfNeeded();
        if (outQueue.size() + hopSize_ > outQueue.capacity()) {
          break;
        }
        for (size_t frame = 0; frame < hopSize_; ++frame) {
          outQueue.push_back(pendingOverlap_[channel][frame] * olaWindow_[hopSize_ + frame]);
          pendingOverlap_[channel][frame] = 0.0f;
        }
      }
      continue;
    }
    break;
  }

  return rendered;
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

  if (firstSynthesisIteration_) {
    firstSynthesisIteration_ = false;
    // Fake the previous grain: its trailing half is this block's leading half
    // (identical content), so the COLA window pair sums to unity and output
    // starts at full amplitude instead of fading in from silence.
    for (size_t channel = 0; channel < channels_; ++channel) {
      for (size_t frame = 0; frame < hopSize_; ++frame) {
        pendingOverlap_[channel][frame] = optimalBlock_[channel][frame];
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

  if (maxSourceIndexForBlock(targetBlockIndex_) >= inputFrames) {
    return false;
  }

  // Last search candidate window starts at searchBlockIndex_ + searchIntervalFrames_ - 1
  // (not searchBlockSize - 1, which double-counted the trailing window).
  if (searchIntervalFrames_ == 0) {
    return false;
  }
  const int lastCandidateStart = searchBlockIndex_ + static_cast<int>(searchIntervalFrames_) - 1;
  return maxSourceIndexForBlock(lastCandidateStart) < inputFrames;
}

bool WsolaTimeStretcher::targetIsWithinSearchRegion() const {
  const int searchBlockSize = static_cast<int>(searchIntervalFrames_ + windowSize_ - 1);
  return targetBlockIndex_ >= searchBlockIndex_ &&
      targetBlockIndex_ + static_cast<int>(windowSize_) <= searchBlockIndex_ + searchBlockSize;
}

void WsolaTimeStretcher::fillSearchSpan() {
  const size_t spanLength = searchIntervalFrames_ + windowSize_;
  for (size_t channel = 0; channel < channels_; ++channel) {
    auto &span = searchSpan_[channel];
    for (size_t j = 0; j < spanLength; ++j) {
      span[j] = sampleAt(channel, searchBlockIndex_ + static_cast<int>(j));
    }
  }
}

int WsolaTimeStretcher::findOptimalBlockIndex() {
  fillBlock(targetBlock_, targetBlockIndex_);
  computeTargetEnergy();
  fillSearchSpan();

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
  const int offset = candidateIndex - searchBlockIndex_;
  float score = 0.0f;

  for (size_t channel = 0; channel < channels_; ++channel) {
    const float *target = targetBlock_[channel].data();
    const float *candidate = searchSpan_[channel].data() + offset;

    const float dot = dsp::dotProduct(target, candidate, windowSize_);
    const float candidateEnergy = dsp::sumOfSquares(candidate, windowSize_);

    score += dot / std::sqrt(targetEnergy_[channel] * candidateEnergy + EPSILON);
  }

  return score;
}

int WsolaTimeStretcher::maxSourceIndexForBlock(int blockStartFrame) const {
  if (windowSize_ == 0) {
    return blockStartFrame;
  }

  const float lastFrame =
      static_cast<float>(blockStartFrame + static_cast<int>(windowSize_) - 1) * pitchFactor_;
  return static_cast<int>(std::ceil(lastFrame));
}

float WsolaTimeStretcher::sampleAt(size_t channel, int frameIndex) const {
  if (frameIndex < 0 || channel >= inputQueue_.size()) {
    return 0.0f;
  }

  const auto &queue = inputQueue_[channel];
  if (queue.empty()) {
    return 0.0f;
  }

  const float sourceIndex = static_cast<float>(frameIndex) * pitchFactor_;
  if (sourceIndex < 0.0f) {
    return 0.0f;
  }

  const float maxIndex = static_cast<float>(queue.size() - 1);
  if (sourceIndex >= maxIndex) {
    return queue.back();
  }

  const auto i0 = static_cast<size_t>(sourceIndex);
  const auto i1 = i0 + 1;
  const float frac = sourceIndex - static_cast<float>(i0);
  return queue[i0] * (1.0f - frac) + queue[i1] * frac;
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
    targetEnergy_[channel] = dsp::sumOfSquares(targetBlock_[channel].data(), windowSize_);
  }
}

void WsolaTimeStretcher::updateOutputTime(float playbackRate, double timeChange) {
  outputTime_ += timeChange;
  // The analysis pointer walks the input in block-index frames, where one block
  // frame maps to pitchFactor_ real input frames (the resample folded into
  // sampleAt). Dividing the advance rate by pitchFactor_ keeps the real input
  // consumed per output frame equal to playbackRate, independent of pitch.
  const double effectiveRate =
      static_cast<double>(playbackRate) / static_cast<double>(pitchFactor_);
  synthesisPosition_ += timeChange * effectiveRate;
  const int searchBlockCenterIndex = static_cast<int>(synthesisPosition_ + 0.5);
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

void WsolaTimeStretcher::compactInputQueue(size_t synthesisFramesToRemove, float playbackRate) {
  if (inputQueue_.empty() || synthesisFramesToRemove == 0) {
    return;
  }

  const size_t queueFramesToRemove = std::min(
      inputQueue_[0].size(),
      static_cast<size_t>(std::floor(static_cast<float>(synthesisFramesToRemove) * pitchFactor_)));

  for (auto &queue : inputQueue_) {
    if (queueFramesToRemove >= queue.size()) {
      queue.clear();
    } else {
      queue.erase(queue.begin(), queue.begin() + static_cast<std::ptrdiff_t>(queueFramesToRemove));
    }
  }

  const int removedFrames = static_cast<int>(synthesisFramesToRemove);
  targetBlockIndex_ -= removedFrames;
  searchBlockIndex_ -= removedFrames;
  synthesisPosition_ = std::max(0.0, synthesisPosition_ - static_cast<double>(removedFrames));
  const double effectiveRate =
      static_cast<double>(playbackRate) / static_cast<double>(pitchFactor_);
  if (effectiveRate > 0.0) {
    outputTime_ = std::max(0.0, outputTime_ - static_cast<double>(removedFrames) / effectiveRate);
  }
}

} // namespace audioapi
