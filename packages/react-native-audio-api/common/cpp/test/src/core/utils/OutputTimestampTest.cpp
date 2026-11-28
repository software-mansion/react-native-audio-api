#include <audioapi/core/utils/OutputTimestamp.h>
#include <gtest/gtest.h>

#include <cstdint>

using namespace audioapi;

namespace {

constexpr float kSampleRate = 48000.0f;
constexpr std::int64_t kMs = 1'000'000;
/// The moment every test reports the position at.
constexpr std::int64_t kNowNs = 3'600'000 * kMs;

double toMs(std::int64_t ns) {
  return static_cast<double>(ns) / kMs;
}

} // namespace

TEST(OutputTimestampTest, ExtrapolatesToNowWhilePlaying) {
  // Frame 48000 (1 s) left the speaker 20 ms ago; the device is now 960 frames on.
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = kNowNs - 20 * kMs};

  const auto timestamp =
      outputTimestampAt(presentation, kSampleRate, /*renderedFrames=*/96000, true, kNowNs);

  EXPECT_NEAR(timestamp.contextTime, 1.020, 1e-9);
  EXPECT_NEAR(timestamp.performanceTime, toMs(kNowNs), 1e-6);
}

TEST(OutputTimestampTest, AcceptsAPresentationStillInTheFuture) {
  // iOS reports the moment a buffer *will* play; looking back from before it is still linear.
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = kNowNs + 10 * kMs};

  const auto timestamp = outputTimestampAt(presentation, kSampleRate, 96000, true, kNowNs);

  EXPECT_NEAR(timestamp.contextTime, 0.990, 1e-9);
  EXPECT_NEAR(timestamp.performanceTime, toMs(kNowNs), 1e-6);
}

TEST(OutputTimestampTest, FreezesWhenTheDeviceIsNotPlaying) {
  const auto presentationTimeNs = kNowNs - 5000 * kMs;
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = presentationTimeNs};

  const auto timestamp = outputTimestampAt(presentation, kSampleRate, 96000, false, kNowNs);

  EXPECT_NEAR(timestamp.contextTime, 1.0, 1e-9);
  EXPECT_NEAR(timestamp.performanceTime, toMs(presentationTimeNs), 1e-6);
}

TEST(OutputTimestampTest, NeverRunsAheadOfRenderedFrames) {
  // A stale report extrapolated over a long stall would claim frames the graph never rendered.
  const auto presentationTimeNs = kNowNs - 2000 * kMs;
  const OutputPresentation presentation{
      .contextFrame = 48000, .presentationTimeNs = presentationTimeNs};

  const auto timestamp =
      outputTimestampAt(presentation, kSampleRate, /*renderedFrames=*/72000, true, kNowNs);

  EXPECT_NEAR(timestamp.contextTime, 1.5, 1e-9);
  // Slid back along the same line: frame 72000 played 0.5 s after frame 48000.
  EXPECT_NEAR(timestamp.performanceTime, toMs(presentationTimeNs) + 500.0, 1e-3);
}

TEST(OutputTimestampTest, NeverReportsANegativeContextTime) {
  // Right after start the device is still playing frames from before the context's first one.
  const auto presentationTimeNs = kNowNs;
  const OutputPresentation presentation{
      .contextFrame = -480, .presentationTimeNs = presentationTimeNs};

  const auto timestamp = outputTimestampAt(presentation, kSampleRate, 4096, true, kNowNs);

  EXPECT_DOUBLE_EQ(timestamp.contextTime, 0.0);
  // Frame 0 reaches the speaker 10 ms after frame -480.
  EXPECT_NEAR(timestamp.performanceTime, toMs(presentationTimeNs) + 10.0, 1e-3);
}

TEST(OutputTimestampTest, KeepsBothCoordinatesOnOneMoment) {
  for (const std::int64_t offsetMs : {-77, 0, 23, 123}) {
    const auto presentationTimeNs = kNowNs + offsetMs * kMs;
    const OutputPresentation presentation{
        .contextFrame = 12345, .presentationTimeNs = presentationTimeNs};

    const auto timestamp = outputTimestampAt(presentation, kSampleRate, 1'000'000, true, kNowNs);
    const double framesFromPresentation =
        (timestamp.performanceTime - toMs(presentationTimeNs)) / 1000.0 * kSampleRate;
    EXPECT_NEAR(timestamp.contextTime * kSampleRate, 12345.0 + framesFromPresentation, 1e-3)
        << "presentation offset = " << offsetMs << " ms";
  }
}
