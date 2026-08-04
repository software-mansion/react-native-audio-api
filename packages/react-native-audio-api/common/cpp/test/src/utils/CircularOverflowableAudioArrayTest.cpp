#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <gtest/gtest.h>

#include <array>

namespace audioapi::test {

TEST(CircularOverflowableAudioArrayTest, WriteThenReadRoundTrips) {
  constexpr size_t kCapacity = 8;
  constexpr size_t kFrames = 4;

  CircularOverflowableAudioArray buffer(kCapacity);
  const std::array<float, kFrames> input = {1.0f, 2.0f, 3.0f, 4.0f};

  buffer.write(input.data(), kFrames);

  std::array<float, kFrames> output = {};
  EXPECT_EQ(buffer.read(output.data(), kFrames), kFrames);
  EXPECT_EQ(output, input);
}

} // namespace audioapi::test
