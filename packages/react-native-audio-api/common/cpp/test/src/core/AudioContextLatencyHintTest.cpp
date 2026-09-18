#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/core/types/AudioContextLatencyHint.h>
#include <gtest/gtest.h>
#include <optional>

using namespace audioapi;

// NOLINTBEGIN

TEST(AudioContextLatencyHintTest, ParsesTheThreeCategories) {
  EXPECT_EQ(
      js_enum_parser::latencyHintFromString("interactive"), AudioContextLatencyHint::INTERACTIVE);
  EXPECT_EQ(js_enum_parser::latencyHintFromString("balanced"), AudioContextLatencyHint::BALANCED);
  EXPECT_EQ(js_enum_parser::latencyHintFromString("playback"), AudioContextLatencyHint::PLAYBACK);
}

TEST(AudioContextLatencyHintTest, TreatsAnUnrecognisedStringAsNoHint) {
  EXPECT_EQ(js_enum_parser::latencyHintFromString(""), std::nullopt);
  EXPECT_EQ(js_enum_parser::latencyHintFromString("foo"), std::nullopt);
  EXPECT_EQ(js_enum_parser::latencyHintFromString("INTERACTIVE"), std::nullopt);
  EXPECT_EQ(js_enum_parser::latencyHintFromString("0.01"), std::nullopt);
}

TEST(AudioContextLatencyHintTest, RequestsNothingWithoutAHintOrForPlayback) {
  EXPECT_EQ(preferredIOBufferFramesFor(std::nullopt), 0);
  EXPECT_EQ(preferredIOBufferFramesFor(AudioContextLatencyHint::PLAYBACK), 0);
}

TEST(AudioContextLatencyHintTest, RequestsOneRenderQuantumForInteractiveAndFourForBalanced) {
  EXPECT_EQ(preferredIOBufferFramesFor(AudioContextLatencyHint::INTERACTIVE), 128);
  EXPECT_EQ(preferredIOBufferFramesFor(AudioContextLatencyHint::BALANCED), 512);
}

// NOLINTEND
