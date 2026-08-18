#include <audioapi/core/utils/RotatingFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

/// In-memory AudioFileWriter that reports a fixed size/duration per closed segment
/// and names segments by open order, so rotation accounting can be asserted exactly.
class StubFileWriter final : public AudioFileWriter {
 public:
  StubFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties)
      : AudioFileWriter(audioEventHandlerRegistry, fileProperties) {}

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer,
      const std::string & /*fileNameOverride*/) override {
    if (open_) {
      return OpenFileResult::Err("file already open");
    }
    open_ = true;
    ++openCount;
    lastSampleRate = streamSampleRate;
    lastChannelCount = streamChannelCount;
    lastMaxFramesPerBuffer = maxFramesPerBuffer;
    path_ = "segment" + std::to_string(openCount);
    return OpenFileResult::Ok(path_);
  }

  CloseFileResult closeFile() override {
    if (!open_) {
      return CloseFileResult::Err("file is not open");
    }
    open_ = false;
    return CloseFileResult::Ok({segmentSizeMB, segmentDurationSec});
  }

  void writeAudioData(const float * /*interleavedFrames*/, int /*numFrames*/) override {}

  [[nodiscard]] std::string getFilePath() const override {
    return path_;
  }
  [[nodiscard]] double getCurrentDuration() const override {
    return 0.0;
  }
  [[nodiscard]] size_t getFileSizeBytes() const override {
    return 0;
  }

  int openCount = 0;
  float lastSampleRate = 0.0F;
  int32_t lastChannelCount = 0;
  int32_t lastMaxFramesPerBuffer = 0;
  double segmentSizeMB = 1.0;
  double segmentDurationSec = 2.0;

 private:
  bool open_ = false;
  std::string path_;
};

} // namespace

class RotatingFileWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    eventRegistry_ = std::make_shared<MockAudioEventHandlerRegistry>();
    properties_ = std::make_shared<AudioFileProperties>(
        AudioFileProperties::FileDirectory::Cache,
        "",
        "test",
        2,
        size_t{1024},
        AudioFileProperties::Format::WAV,
        48000.0F,
        size_t{128000},
        AudioFileProperties::BitDepth::Bit16,
        5,
        0,
        AudioFileProperties::IOSAudioQuality::High);

    rotatingWriter_ = std::make_shared<RotatingFileWriter>(
        eventRegistry_,
        properties_,
        properties_->rotateIntervalBytes,
        [this](const std::shared_ptr<AudioFileProperties> &props) {
          auto writer = std::make_shared<StubFileWriter>(eventRegistry_, props);
          stubWriter_ = writer.get();
          return writer;
        },
        [this](const std::string &path) { openedSegmentPaths_.push_back(path); });
  }

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry_;
  std::shared_ptr<AudioFileProperties> properties_;
  std::shared_ptr<RotatingFileWriter> rotatingWriter_;
  StubFileWriter *stubWriter_ = nullptr;
  std::vector<std::string> openedSegmentPaths_;
};

TEST_F(RotatingFileWriterTest, ReprepareStreamFormatOpensSegmentWithNewFormat) {
  auto openResult = rotatingWriter_->openFile(48000.0F, 2, 128, "");
  ASSERT_TRUE(openResult.is_ok());
  EXPECT_EQ(openResult.unwrap(), "segment1");

  auto reprepareResult = rotatingWriter_->reprepareStreamFormat(44100.0F, 1, 256);
  ASSERT_TRUE(reprepareResult.is_ok());
  EXPECT_EQ(reprepareResult.unwrap(), "segment2");

  ASSERT_NE(stubWriter_, nullptr);
  EXPECT_EQ(stubWriter_->openCount, 2);
  EXPECT_FLOAT_EQ(stubWriter_->lastSampleRate, 44100.0F);
  EXPECT_EQ(stubWriter_->lastChannelCount, 1);
  EXPECT_EQ(stubWriter_->lastMaxFramesPerBuffer, 256);

  const std::vector<std::string> expectedPaths{"segment1", "segment2"};
  EXPECT_EQ(openedSegmentPaths_, expectedPaths);
}

TEST_F(RotatingFileWriterTest, ReprepareStreamFormatPreservesCumulativeTotals) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "").is_ok());
  ASSERT_TRUE(rotatingWriter_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  auto closeResult = rotatingWriter_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  // One segment closed by the reprepare plus the final one: totals must cover both.
  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(std::get<0>(totals), 2.0 * stubWriter_->segmentSizeMB);
  EXPECT_DOUBLE_EQ(std::get<1>(totals), 2.0 * stubWriter_->segmentDurationSec);
}

TEST_F(RotatingFileWriterTest, ReprepareStreamFormatWithoutOpenFails) {
  auto result = rotatingWriter_->reprepareStreamFormat(44100.0F, 1, 256);
  ASSERT_TRUE(result.is_err());
}

// NOLINTEND
