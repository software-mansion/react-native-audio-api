#include <audioapi/core/utils/EncodedAudioFileWriter.h>
#include <audioapi/core/utils/RotatingFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

/// Stands in for the platform encoder writer. It derives from EncodedAudioFileWriter because
/// RotatingFileWriter reaches switchToFile() by casting to that type, and overrides every step
/// that would otherwise need a system encoder and a real recording directory — neither of which
/// exists in the desktop test build. Sizes and durations are fixed per segment so rotation
/// accounting can be asserted exactly, and segments are named by open order.
class StubFileWriter final : public EncodedAudioFileWriter {
 public:
  StubFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties)
      : EncodedAudioFileWriter(audioEventHandlerRegistry, fileProperties) {}

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer,
      const std::string &fileNameOverride) override {
    if (open_) {
      return OpenFileResult::Err("file already open");
    }
    open_ = true;
    lastSampleRate = streamSampleRate;
    lastChannelCount = streamChannelCount;
    lastMaxFramesPerBuffer = maxFramesPerBuffer;
    return OpenFileResult::Ok(beginSegment(fileNameOverride));
  }

  CloseFileResult closeFile() override {
    if (!open_) {
      return CloseFileResult::Err("file is not open");
    }
    open_ = false;
    return CloseFileResult::Ok({segmentSizeMB, segmentDurationSec});
  }

  CloseFileResult switchToFile(const std::string &fileNameOverride) override {
    if (!open_) {
      return CloseFileResult::Err("file is not open");
    }
    if (failNextSwitch) {
      return CloseFileResult::Err("switch failed");
    }
    beginSegment(fileNameOverride);
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
    return fileSizeBytes;
  }

  /// Stands in for one buffer reaching the worker thread and being encoded there, which is
  /// where a real writer gives rotation its chance to run.
  void encodeBuffer() {
    notifyBufferEncoded();
  }

  void assignOnErrorCallbackId(uint64_t callbackId) override {
    EncodedAudioFileWriter::assignOnErrorCallbackId(callbackId);
    lastErrorCallbackId = callbackId;
  }

  uint64_t lastErrorCallbackId = 0;

  size_t fileSizeBytes = 0;
  bool failNextSwitch = false;
  int openCount = 0;
  std::vector<std::string> openedStems;
  float lastSampleRate = 0.0F;
  int32_t lastChannelCount = 0;
  int32_t lastMaxFramesPerBuffer = 0;
  double segmentSizeMB = 1.0;
  double segmentDurationSec = 2.0;

 private:
  std::string beginSegment(const std::string &fileNameOverride) {
    ++openCount;
    openedStems.push_back(fileNameOverride);
    path_ = "segment" + std::to_string(openCount);
    return path_;
  }

  bool open_ = false;
  std::string path_;
};

// Mirrors RotatingFileWriter::FILE_SIZE_CHECK_WRITE_INTERVAL.
constexpr int kBuffersBetweenSizeChecks = 10;

void encodeBuffers(StubFileWriter &writer, int count) {
  for (int buffer = 0; buffer < count; ++buffer) {
    writer.encodeBuffer();
  }
}

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

    auto stubWriter = std::make_shared<StubFileWriter>(eventRegistry_, properties_);
    stubWriter_ = stubWriter.get();

    rotatingWriter_ = std::make_shared<RotatingFileWriter>(
        eventRegistry_,
        properties_,
        properties_->rotateIntervalBytes,
        std::move(stubWriter),
        [this](const std::string &path) { openedSegmentPaths_.push_back(path); });
  }

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry_;
  std::shared_ptr<AudioFileProperties> properties_;
  std::shared_ptr<RotatingFileWriter> rotatingWriter_;
  StubFileWriter *stubWriter_ = nullptr;
  std::vector<std::string> openedSegmentPaths_;
};

TEST_F(RotatingFileWriterTest, ReprepareStreamFormatOpensSegmentWithNewFormat) {
  auto openResult = rotatingWriter_->openFile(48000.0F, 2, 128, "session");
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

TEST_F(RotatingFileWriterTest, SegmentsAreNumberedFromTheSessionStem) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  ASSERT_TRUE(rotatingWriter_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  ASSERT_NE(stubWriter_, nullptr);
  const std::vector<std::string> expectedStems{"session_001", "session_002"};
  EXPECT_EQ(stubWriter_->openedStems, expectedStems);
}

TEST_F(RotatingFileWriterTest, ReprepareStreamFormatPreservesCumulativeTotals) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
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

TEST_F(RotatingFileWriterTest, WriteAudioDataNeverRotates) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  stubWriter_->fileSizeBytes = properties_->rotateIntervalBytes * 4;

  const std::vector<float> frames(256, 0.0F);
  for (int write = 0; write < kBuffersBetweenSizeChecks * 4; ++write) {
    rotatingWriter_->writeAudioData(frames.data(), 128);
  }

  // Rotation belongs to the worker thread; the audio thread may only hand the buffer over.
  EXPECT_EQ(stubWriter_->openCount, 1);
}

TEST_F(RotatingFileWriterTest, RotatesOnceTheSegmentOutgrowsTheCap) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  stubWriter_->fileSizeBytes = properties_->rotateIntervalBytes + 1;

  encodeBuffers(*stubWriter_, kBuffersBetweenSizeChecks);

  EXPECT_EQ(stubWriter_->openCount, 2);
  const std::vector<std::string> expectedStems{"session_001", "session_002"};
  EXPECT_EQ(stubWriter_->openedStems, expectedStems);

  const std::vector<std::string> expectedPaths{"segment1", "segment2"};
  EXPECT_EQ(openedSegmentPaths_, expectedPaths);
}

TEST_F(RotatingFileWriterTest, SegmentSizeIsMeasuredOnlyEveryNthBuffer) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  stubWriter_->fileSizeBytes = properties_->rotateIntervalBytes + 1;

  encodeBuffers(*stubWriter_, kBuffersBetweenSizeChecks - 1);

  EXPECT_EQ(stubWriter_->openCount, 1);
}

TEST_F(RotatingFileWriterTest, SegmentUnderTheCapIsLeftAlone) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  stubWriter_->fileSizeBytes = properties_->rotateIntervalBytes;

  encodeBuffers(*stubWriter_, kBuffersBetweenSizeChecks * 3);

  EXPECT_EQ(stubWriter_->openCount, 1);
}

TEST_F(RotatingFileWriterTest, RotationFoldsRetiredSegmentsIntoTheTotals) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  stubWriter_->fileSizeBytes = properties_->rotateIntervalBytes + 1;

  encodeBuffers(*stubWriter_, kBuffersBetweenSizeChecks);
  ASSERT_EQ(stubWriter_->openCount, 2);

  auto closeResult = rotatingWriter_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  // The rotated segment plus the final one.
  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(std::get<0>(totals), 2.0 * stubWriter_->segmentSizeMB);
  EXPECT_DOUBLE_EQ(std::get<1>(totals), 2.0 * stubWriter_->segmentDurationSec);
}

TEST_F(RotatingFileWriterTest, ErrorCallbackReachesTheSegmentWriter) {
  rotatingWriter_->setOnErrorCallback(42);
  EXPECT_EQ(stubWriter_->lastErrorCallbackId, 42U);

  rotatingWriter_->clearOnErrorCallback();
  EXPECT_EQ(stubWriter_->lastErrorCallbackId, 0U);
}

TEST_F(RotatingFileWriterTest, AFailedRotationStopsTheWriter) {
  ASSERT_TRUE(rotatingWriter_->openFile(48000.0F, 2, 128, "session").is_ok());
  stubWriter_->fileSizeBytes = properties_->rotateIntervalBytes + 1;
  stubWriter_->failNextSwitch = true;

  encodeBuffers(*stubWriter_, kBuffersBetweenSizeChecks);

  EXPECT_EQ(stubWriter_->openCount, 1);
  EXPECT_TRUE(rotatingWriter_->closeFile().is_err());
}

// NOLINTEND
