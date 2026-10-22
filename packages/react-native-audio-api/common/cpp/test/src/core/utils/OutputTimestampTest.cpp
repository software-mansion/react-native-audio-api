#include <audioapi/core/utils/OutputTimestamp.h>
#include <gtest/gtest.h>

#include <chrono>

using namespace audioapi;

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr std::int64_t kMs = 1'000'000;
/// Real time passing between the test reading the clock and the function reading it.
constexpr double kClockSkewMs = 5.0;

std::int64_t steadyNowNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

double toMs(std::int64_t ns) {
  return static_cast<double>(ns) / kMs;
}

} // namespace

TEST(OutputTimestampTest, ExtrapolatesToNowWhilePlaying) {
  // Frame 48000 (1 s) left the speaker 20 ms ago; the device is now ~960 frames on.
  const auto nowNs = steadyNowNs();
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = nowNs - 20 * kMs};

  const auto timestamp =
      outputTimestampAt(presentation, kSampleRate, /*renderedFrames=*/96000, true);

  EXPECT_NEAR(timestamp.contextTime, 1.020, kClockSkewMs / 1000.0);
  EXPECT_NEAR(timestamp.performanceTime, toMs(nowNs), kClockSkewMs);
}

TEST(OutputTimestampTest, AcceptsAPresentationStillInTheFuture) {
  // iOS reports the moment a buffer *will* play; looking back from before it is still linear.
  const auto nowNs = steadyNowNs();
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = nowNs + 10 * kMs};

  const auto timestamp = outputTimestampAt(presentation, kSampleRate, 96000, true);

  EXPECT_NEAR(timestamp.contextTime, 0.990, kClockSkewMs / 1000.0);
  EXPECT_NEAR(timestamp.performanceTime, toMs(nowNs), kClockSkewMs);
}

TEST(OutputTimestampTest, FreezesWhenTheDeviceIsNotPlaying) {
  const auto presentationTimeNs = steadyNowNs() - 5000 * kMs;
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = presentationTimeNs};

  const auto timestamp = outputTimestampAt(presentation, kSampleRate, 96000, false);

  EXPECT_NEAR(timestamp.contextTime, 1.0, 1e-9);
  EXPECT_NEAR(timestamp.performanceTime, toMs(presentationTimeNs), 1e-6);
}

TEST(OutputTimestampTest, NeverRunsAheadOfRenderedFrames) {
  // A stale report extrapolated over a long stall would claim frames the graph never rendered.
  const auto presentationTimeNs = steadyNowNs() - 2000 * kMs;
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = presentationTimeNs};

  const auto timestamp =
      outputTimestampAt(presentation, kSampleRate, /*renderedFrames=*/72000, true);

  EXPECT_NEAR(timestamp.contextTime, 1.5, 1e-9);
  // Slid back along the same line: frame 72000 played 0.5 s after frame 48000.
  EXPECT_NEAR(timestamp.performanceTime, toMs(presentationTimeNs) + 500.0, 1e-3);
}

TEST(OutputTimestampTest, NeverReportsANegativeContextTime) {
  // Right after start the device is still playing frames from before the context's first one.
  const auto presentationTimeNs = steadyNowNs();
  const OutputPresentation presentation{
      .contextFrame = -480, .presentationTimeNs = presentationTimeNs};

  const auto timestamp = outputTimestampAt(presentation, kSampleRate, 4096, true);

  EXPECT_DOUBLE_EQ(timestamp.contextTime, 0.0);
  // Frame 0 reaches the speaker 10 ms after frame -480.
  EXPECT_NEAR(timestamp.performanceTime, toMs(presentationTimeNs) + 10.0, 1e-3);
}

TEST(OutputTimestampTest, KeepsBothCoordinatesOnOneMoment) {
  const auto nowNs = steadyNowNs();

  for (const std::int64_t offsetMs : {-77, 0, 23, 123}) {
    const auto presentationTimeNs = nowNs + offsetMs * kMs;
    const OutputPresentation presentation{
        .contextFrame = 12345, .presentationTimeNs = presentationTimeNs};

    const auto timestamp = outputTimestampAt(presentation, kSampleRate, 1'000'000, true);
    const double framesFromPresentation =
        (timestamp.performanceTime - toMs(presentationTimeNs)) / 1000.0 * kSampleRate;
    EXPECT_NEAR(timestamp.contextTime * kSampleRate, 12345.0 + framesFromPresentation, 1e-3)
        << "presentation offset = " << offsetMs << " ms";
  }
}
