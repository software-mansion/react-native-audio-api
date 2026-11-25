#include <audioapi/utils/CircularArray.hpp>
#include <gtest/gtest.h>

#include <array>
#include <numeric>

namespace audioapi::test {

// A ring exactly one window long, fed in blocks that do not divide it, always reads back the
// newest window in order: the way UIWorkletSnapshots and AnalyserNode use it.
TEST(CircularArrayTest, PopBackReturnsNewestWindowAcrossWraps) {
  constexpr size_t kWindow = 8;
  constexpr size_t kBlock = 3;
  CircularAudioArray ring(kWindow);

  float nextSample = 0.0F;
  for (int block = 0; block < 7; ++block) {
    std::array<float, kBlock> samples{};
    std::iota(samples.begin(), samples.end(), nextSample);
    nextSample += kBlock;
    ring.push_back(samples.data(), kBlock, true);

    if (nextSample < kWindow) {
      continue;
    }
    std::array<float, kWindow> window{};
    ring.pop_back(window.data(), kWindow, 0, true);

    std::array<float, kWindow> expected{};
    std::iota(expected.begin(), expected.end(), nextSample - kWindow);
    EXPECT_EQ(window, expected) << "after " << nextSample << " samples";
  }
}

} // namespace audioapi::test
