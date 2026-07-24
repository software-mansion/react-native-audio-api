#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <optional>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr float kSampleRate = 44100.0f;
constexpr float kPlaybackRate = 1.5f;
constexpr size_t kQuantum = RENDER_QUANTUM_SIZE;

size_t inputFramesForQuantum(float playbackRate) {
  return static_cast<size_t>(std::max(1, static_cast<int>(std::ceil(playbackRate * kQuantum))));
}

float schedulingLatencySeconds(float playbackRate) {
  return (WsolaTimeStretcher::OUTPUT_LATENCY_MS / 1000.0f) +
      (WsolaTimeStretcher::INPUT_LATENCY_MS / 1000.0f) * playbackRate;
}

void recordFirstPeakIfAbsent(
    const DSPAudioBuffer &output,
    size_t totalOutputFrames,
    size_t inputFramesFed,
    std::optional<size_t> &firstPeakAtOutputFrame,
    size_t &inputFramesWhenPeak,
    float threshold = 0.01f) {
  if (firstPeakAtOutputFrame.has_value()) {
    return;
  }

  const auto outSpan = output.getChannel(0)->span();
  for (size_t i = 0; i < outSpan.size(); ++i) {
    if (std::abs(outSpan[i]) > threshold) {
      firstPeakAtOutputFrame = totalOutputFrames + i;
      inputFramesWhenPeak = inputFramesFed;
      return;
    }
  }
}

size_t fillToneBurstChunk(
    DSPAudioBuffer &input,
    size_t inputFrames,
    size_t inputPos,
    size_t maxInputFrames,
    float burstSeconds,
    float sampleRate) {
  auto channel = input.getChannel(0)->span();
  for (size_t i = 0; i < inputFrames && inputPos < maxInputFrames; ++i, ++inputPos) {
    const float t = static_cast<float>(inputPos) / sampleRate;
    if (t < burstSeconds) {
      channel[i] = std::sin(2.0f * PI * 440.0f * t);
    }
  }
  return inputPos;
}

} // namespace

class WsolaTimeStretcherTest : public ::testing::Test {
 protected:
  WsolaTimeStretcher stretcher_;

  void SetUp() override {
    stretcher_.configure(1, kSampleRate);
  }
};

TEST_F(WsolaTimeStretcherTest, RequiredInputFramesAt44100Hz) {
  // OLA window 20ms + search interval 30ms at 44.1 kHz.
  EXPECT_EQ(stretcher_.getRequiredInputFrames(), 2205u);
}

TEST_F(WsolaTimeStretcherTest, MinInputFramesToRunMatchesSearchSpanNeed) {
  // Cold start: last candidate needs searchInterval + window - 1 source samples.
  EXPECT_EQ(stretcher_.getMinInputFramesToRun(), stretcher_.getRequiredInputFrames() - 1u);
  EXPECT_LE(stretcher_.getMinInputFramesToRun(), stretcher_.getRequiredInputFrames());
}

TEST_F(WsolaTimeStretcherTest, FeedInputPrimesForImmediateFirstQuantumOutput) {
  const size_t framesNeeded = stretcher_.getMinInputFramesToRun();
  ASSERT_GT(framesNeeded, 0u);

  DSPAudioBuffer prime(framesNeeded, 1, kSampleRate);
  for (size_t i = 0; i < framesNeeded; ++i) {
    prime.getChannel(0)->span()[i] =
        std::sin(2.0f * PI * 440.0f * static_cast<float>(i) / kSampleRate);
  }
  stretcher_.feedInput(prime, framesNeeded);
  EXPECT_GE(stretcher_.getBufferedInputFrames(), framesNeeded);

  // One quantum of additional input at rate 1.5 — should emit non-zero in the first process().
  const size_t inputFrames = inputFramesForQuantum(kPlaybackRate);
  DSPAudioBuffer input(inputFrames, 1, kSampleRate);
  DSPAudioBuffer output(kQuantum, 1, kSampleRate);
  for (size_t i = 0; i < inputFrames; ++i) {
    input.getChannel(0)->span()[i] =
        std::sin(2.0f * PI * 440.0f * static_cast<float>(framesNeeded + i) / kSampleRate);
  }

  stretcher_.process(input, inputFrames, output, kQuantum, kPlaybackRate);

  float peak = 0.0f;
  for (size_t i = 0; i < output.getSize(); ++i) {
    peak = std::max(peak, std::abs(output.getChannel(0)->span()[i]));
  }
  EXPECT_GT(peak, 0.01f);
}

TEST_F(WsolaTimeStretcherTest, SchedulingLatencyFormulaMatchesDocumentedConstants) {
  EXPECT_NEAR(schedulingLatencySeconds(1.0f), 0.03f, 1e-6f);
  EXPECT_NEAR(schedulingLatencySeconds(kPlaybackRate), 0.04f, 1e-6f);
}

TEST_F(WsolaTimeStretcherTest, ProducesOutputWhenPlaybackRateIsNotUnity) {
  DSPAudioBuffer input(inputFramesForQuantum(kPlaybackRate), 1, kSampleRate);
  DSPAudioBuffer output(kQuantum, 1, kSampleRate);

  float peak = 0.0f;
  size_t globalInputFrame = 0;

  for (size_t pass = 0; pass < 40 && peak <= 0.01f; ++pass) {
    for (size_t i = 0; i < input.getSize(); ++i, ++globalInputFrame) {
      input.getChannel(0)->span()[i] =
          std::sin(2.0f * PI * 440.0f * static_cast<float>(globalInputFrame) / kSampleRate);
    }

    stretcher_.process(input, input.getSize(), output, output.getSize(), kPlaybackRate);

    for (size_t i = 0; i < output.getSize(); ++i) {
      peak = std::max(peak, std::abs(output.getChannel(0)->span()[i]));
    }
  }

  EXPECT_GT(peak, 0.01f);
}

TEST_F(WsolaTimeStretcherTest, ToneBurstPassesThroughWithBoundedLatency) {
  constexpr size_t kMaxInputFrames = 44100;
  constexpr float kBurstSeconds = 0.1f;
  const size_t requiredInput = stretcher_.getRequiredInputFrames();

  size_t inputPos = 0;
  size_t totalOutputFrames = 0;
  std::optional<size_t> firstPeakAtOutputFrame;
  size_t inputFramesWhenPeak = 0;

  while (inputPos < kMaxInputFrames) {
    const size_t inputFrames = inputFramesForQuantum(kPlaybackRate);
    DSPAudioBuffer input(inputFrames, 1, kSampleRate);
    DSPAudioBuffer output(kQuantum, 1, kSampleRate);
    input.zero();

    inputPos = fillToneBurstChunk(
        input, inputFrames, inputPos, kMaxInputFrames, kBurstSeconds, kSampleRate);

    stretcher_.process(input, inputFrames, output, kQuantum, kPlaybackRate);

    recordFirstPeakIfAbsent(
        output, totalOutputFrames, inputPos, firstPeakAtOutputFrame, inputFramesWhenPeak);
    totalOutputFrames += kQuantum;

    // Break out of the loop if we've detected a peak and the input position is past twice the required input frames.
    if (firstPeakAtOutputFrame && inputPos > requiredInput * 2) {
      break;
    }
  }

  ASSERT_TRUE(firstPeakAtOutputFrame.has_value()) << "WSOLA never produced tone burst energy";

  const float measuredInputLatencySec = static_cast<float>(inputFramesWhenPeak) / kSampleRate;
  const float primingLatencySec = static_cast<float>(requiredInput) / kSampleRate;

  // WSOLA must prime its analysis window before emitting audio; delay should stay within
  // the configured priming budget plus a small processing margin.
  EXPECT_LT(measuredInputLatencySec, primingLatencySec + 0.15f);
}

// NOLINTEND
