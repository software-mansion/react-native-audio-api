#include <audioapi/core/inputs/RecorderOutputs.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <test/src/utils/FakeFileBackend.h>

#include <cstdint>
#include <memory>
#include <vector>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

class RecorderOutputsTest : public ::testing::Test {
 protected:
  /// Feeds one buffer of FRAMES mono frames, as the audio thread does.
  void writeFrames() {
    const std::vector<float> samples(FRAMES, 0.25F);
    const float *channels[] = {samples.data()};
    outputs_.write(channels, FRAMES);
  }

  /// Ends the session the way the recorder does, so every worker is joined.
  Result<FileInfo, std::string> endSession() {
    return RecorderOutputs::finalize(outputs_.detach());
  }

  /// Expects exactly @p count buffers to reach the JS callback. Exact only once the session is
  /// ended, which joins the callback's worker thread and flushes what it still holds.
  void expectAudioReadyEvents(int count) {
    EXPECT_CALL(*registry_, dispatchEvent(AudioEvent::AUDIO_READY, CALLBACK_ID, testing::_))
        .Times(count);
  }

  static constexpr int FRAMES = 128;
  static constexpr uint64_t CALLBACK_ID = 99;
  static constexpr StreamFormat MONO_48K{
      .layout = {.sampleRate = 48000.0F, .channelCount = 1},
      .maxFramesPerBuffer = 256,
  };

  std::shared_ptr<testing::NiceMock<MockAudioEventHandlerRegistry>> registry_ =
      std::make_shared<testing::NiceMock<MockAudioEventHandlerRegistry>>();
  FakeEncoderLog log_;
  PlatformFileBackend backend_ = makeFakeBackend(log_);
  RecorderOutputs outputs_{registry_, backend_};
};

TEST_F(RecorderOutputsTest, InvalidFilePropertiesAreRejectedAndNoFileIsWritten) {
  auto properties = wavFileProperties();
  properties->fileLayout.sampleRate = 0.0F;

  EXPECT_TRUE(outputs_.enableFile(properties).is_err());

  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  auto session = endSession();
  ASSERT_TRUE(session.is_ok());
  EXPECT_TRUE(session.unwrap().paths.empty());
}

TEST(RecorderOutputsPlatformTest, AFormatThePlatformCannotWriteIsRejected) {
  RecorderOutputs outputs(nullptr, osFileBackend());
  auto properties = wavFileProperties();
  properties->encoding.format = AudioFileProperties::FileFormat::OPUS_WEBM;

  EXPECT_TRUE(outputs.enableFile(properties).is_err());
}

TEST_F(RecorderOutputsTest, OutputsThatWereNeverRequestedStayOffAcrossSessions) {
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());

  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  EXPECT_TRUE(endSession().unwrap().paths.empty());
}

TEST_F(RecorderOutputsTest, ACallbackIsFedOnlyOnceASessionPreparesIt) {
  expectAudioReadyEvents(1);
  ASSERT_TRUE(outputs_.setCallback(48000.0F, FRAMES, 1, CALLBACK_ID, std::nullopt).is_ok());
  writeFrames();

  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderOutputsTest, ACallbackThatCannotBePreparedMidSessionIsNotKept) {
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());

  EXPECT_TRUE(outputs_.setCallback(/*sampleRate=*/0.0F, FRAMES, 1, CALLBACK_ID, MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  // A callback still requested would fail this prepare again.
  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderOutputsTest, AConnectionWhoseAdapterNodeIsGoneFailsEverySession) {
  outputs_.connect(std::make_shared<utils::graph::NodeHandle>(nullptr), std::nullopt);

  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  // Still requested: only disconnect() withdraws it.
  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_err());
  ASSERT_TRUE(endSession().is_ok());

  RecorderOutputs::finalize(outputs_.disconnect());
  EXPECT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  ASSERT_TRUE(endSession().is_ok());
}

TEST_F(RecorderOutputsTest, DisablingTheFileClosesItMidSession) {
  ASSERT_TRUE(outputs_.enableFile(wavFileProperties()).is_ok());
  ASSERT_TRUE(outputs_.prepare(MONO_48K).is_ok());
  writeFrames();

  auto disabled = RecorderOutputs::finalize(outputs_.disableFile());
  writeFrames();

  ASSERT_TRUE(disabled.is_ok());
  EXPECT_EQ(disabled.unwrap().paths.size(), 1U);
  EXPECT_EQ(log_.encodedBuffers.load(), 1);
  EXPECT_TRUE(endSession().unwrap().paths.empty());
}

// NOLINTEND
