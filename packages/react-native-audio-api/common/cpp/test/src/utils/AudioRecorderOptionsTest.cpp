#include <audioapi/utils/AudioRecorderOptions.h>
#include <gtest/gtest.h>

#include <string>

using namespace audioapi;

// NOLINTBEGIN

using AndroidInputPreset = AudioRecorderOptions::AndroidInputPreset;

TEST(AudioRecorderOptionsTest, ParsesEveryJsPresetName) {
  EXPECT_EQ(
      AudioRecorderOptions::parseAndroidInputPreset("generic").unwrap(),
      AndroidInputPreset::Generic);
  EXPECT_EQ(
      AudioRecorderOptions::parseAndroidInputPreset("camcorder").unwrap(),
      AndroidInputPreset::Camcorder);
  EXPECT_EQ(
      AudioRecorderOptions::parseAndroidInputPreset("voiceRecognition").unwrap(),
      AndroidInputPreset::VoiceRecognition);
  EXPECT_EQ(
      AudioRecorderOptions::parseAndroidInputPreset("voiceCommunication").unwrap(),
      AndroidInputPreset::VoiceCommunication);
  EXPECT_EQ(
      AudioRecorderOptions::parseAndroidInputPreset("unprocessed").unwrap(),
      AndroidInputPreset::Unprocessed);
  EXPECT_EQ(
      AudioRecorderOptions::parseAndroidInputPreset("voicePerformance").unwrap(),
      AndroidInputPreset::VoicePerformance);
}

TEST(AudioRecorderOptionsTest, RejectsAnUnknownPresetName) {
  EXPECT_TRUE(AudioRecorderOptions::parseAndroidInputPreset("voicecommunication").is_err());
  EXPECT_TRUE(AudioRecorderOptions::parseAndroidInputPreset("").is_err());
}

TEST(AudioRecorderOptionsTest, DefaultsLeaveThePlatformChainAlone) {
  AudioRecorderOptions options;
  EXPECT_EQ(options.androidInputPreset, AndroidInputPreset::PlatformDefault);
  EXPECT_FALSE(options.iosVoiceProcessing);
}

// NOLINTEND
