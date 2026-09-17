#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

/// Shared by every encoder a session opens. Written on the worker thread; read by the test
/// after closeFile(), which joins that worker.
struct FakeEncoderLog {
  std::atomic<size_t> fileSizeBytes{0};
  std::atomic<int> encodedBuffers{0};
  std::atomic<bool> failNextOpen{false};
  double fileSizeMB = 1.0;
  double fileDurationSec = 2.0;

  std::mutex mutex;
  std::vector<std::string> openedFileNames;
  std::vector<std::string> openedPaths;
  StreamFormat lastOpenedFormat{};
  size_t lastOpenedMaxFramesPerBuffer = 0;
  EncoderOutputSpec outputSpec{
      .container = AudioContainer::WAV,
      .codec = AudioCodec::PCM,
      .extension = "wav"};
};

class FakeEncoder final : public AudioEncoder {
 public:
  FakeEncoder(const std::shared_ptr<AudioFileProperties> &fileProperties, FakeEncoderLog &log)
      : AudioEncoder(fileProperties), log_(log) {}

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec & /*outputSpec*/,
      size_t maxBufferSizeInFrames,
      const std::string &filePath) override {
    if (log_.failNextOpen.exchange(false)) {
      return OpenEncoderResult::Err("encoder refused to open");
    }
    inputFormat_ = inputFormat;
    maxBufferSizeInFrames_ = maxBufferSizeInFrames;
    filePath_ = filePath;
    {
      std::scoped_lock lock(log_.mutex);
      log_.lastOpenedFormat = inputFormat;
      log_.lastOpenedMaxFramesPerBuffer = maxBufferSizeInFrames;
    }
    markOpen();
    return OpenEncoderResult::Ok(filePath);
  }

  EncodeResult encode(const void * /*data*/, int numFrames) override {
    addEncodedFrames(static_cast<size_t>(numFrames));
    log_.encodedBuffers.fetch_add(1, std::memory_order_acq_rel);
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    markClosed();
    return CloseEncoderResult::Ok({log_.fileSizeMB, log_.fileDurationSec});
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return log_.fileSizeBytes.load(std::memory_order_acquire);
  }

 private:
  FakeEncoderLog &log_;
};

/// Stands in for the platform steps the desktop test build cannot perform. Resolved paths are
/// the bare file names, so a test can assert on them directly.
PlatformFileBackend makeFakeBackend(FakeEncoderLog &log) {
  return PlatformFileBackend{
      .resolveOutputSpec =
          [&log](AudioFileProperties::Format /*format*/) {
            std::scoped_lock lock(log.mutex);
            return Result<EncoderOutputSpec, std::string>::Ok(log.outputSpec);
          },
      .resolvePath =
          [&log](
              const std::shared_ptr<AudioFileProperties> & /*properties*/,
              const std::string &fileName) {
            std::scoped_lock lock(log.mutex);
            log.openedFileNames.push_back(fileName);
            return Result<std::string, std::string>::Ok(fileName);
          },
      .createEncoder = [&log](const std::shared_ptr<AudioFileProperties> &properties)
          -> std::unique_ptr<AudioEncoder> {
        return std::make_unique<FakeEncoder>(properties, log);
      },
  };
}

// Mirror AudioFileWriter::FILE_SIZE_CHECK_WRITE_INTERVAL and FILE_WRITER_POOL_SIZE.
constexpr int kBuffersBetweenSizeChecks = 10;
constexpr int kWriterPoolSize = 32;
constexpr int kFramesPerBuffer = 128;
constexpr int kChannelCount = 2;
constexpr size_t kRotateIntervalBytes = 1024;

} // namespace

class AudioFileWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    eventRegistry_ = std::make_shared<MockAudioEventHandlerRegistry>();
    frames_.assign(static_cast<size_t>(kFramesPerBuffer * kChannelCount), 0.0F);
  }

  void createWriter(bool rotates) {
    properties_ = std::make_shared<AudioFileProperties>(
        AudioFileProperties::FileDirectory::Cache,
        "",
        "session",
        kChannelCount,
        rotates ? kRotateIntervalBytes : size_t{0},
        AudioFileProperties::Format::WAV,
        48000.0F,
        size_t{128000},
        AudioFileProperties::BitDepth::Bit16,
        5,
        0,
        AudioFileProperties::IOSAudioQuality::High);

    writer_ = std::make_unique<AudioFileWriter>(
        eventRegistry_,
        properties_,
        [this](const std::string &path) {
          std::scoped_lock lock(log_.mutex);
          log_.openedPaths.push_back(path);
        },
        makeFakeBackend(log_));
  }

  OpenFileResult open() {
    return writer_->openFile(48000.0F, kChannelCount, kFramesPerBuffer);
  }

  /// Within the pool size, no buffer is dropped whatever the worker's pace.
  void writeBuffers(int count) {
    ASSERT_LE(count, kWriterPoolSize);
    for (int buffer = 0; buffer < count; ++buffer) {
      writer_->writeAudioData(frames_.data(), kFramesPerBuffer);
    }
  }

  std::vector<std::string> openedFileNames() {
    std::scoped_lock lock(log_.mutex);
    return log_.openedFileNames;
  }

  std::vector<std::string> openedPaths() {
    std::scoped_lock lock(log_.mutex);
    return log_.openedPaths;
  }

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry_;
  std::shared_ptr<AudioFileProperties> properties_;
  FakeEncoderLog log_;
  std::unique_ptr<AudioFileWriter> writer_;
  std::vector<float> frames_;
};

TEST_F(AudioFileWriterTest, OpensTheSessionUnderItsPlainName) {
  createWriter(/*rotates=*/false);

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());
  EXPECT_EQ(openResult.unwrap(), "session.wav");
  EXPECT_EQ(writer_->getFilePath(), "session.wav");

  const std::vector<std::string> expectedPaths{"session.wav"};
  EXPECT_EQ(openedPaths(), expectedPaths);
}

// The platform capability table is empty unless the host is iOS or Android, so a writer that
// consulted it could not open any file on a Linux CI runner while passing on a macOS desktop.
// Opus in WebM is absent from every platform's table, so this pins the writer to its backend.
TEST_F(AudioFileWriterTest, NamesTheFileFromTheBackendSpecNotThePlatformTable) {
  createWriter(/*rotates=*/false);
  log_.outputSpec = {
      .container = AudioContainer::WEBM, .codec = AudioCodec::OPUS, .extension = "webm"};

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());
  EXPECT_EQ(openResult.unwrap(), "session.webm");
}

TEST_F(AudioFileWriterTest, OpenFailsWhileAFileIsOpen) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  EXPECT_TRUE(open().is_err());
  EXPECT_EQ(openedFileNames().size(), 1U);
}

TEST_F(AudioFileWriterTest, CloseWithoutOpenFails) {
  createWriter(/*rotates=*/false);
  EXPECT_TRUE(writer_->closeFile().is_err());
}

TEST_F(AudioFileWriterTest, QueuedBuffersAreEncodedBeforeTheFileCloses) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  writeBuffers(5);
  ASSERT_TRUE(writer_->closeFile().is_ok());

  EXPECT_EQ(log_.encodedBuffers.load(), 5);
}

TEST_F(AudioFileWriterTest, DurationCountsEveryFileOfTheSession) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(48000.0F, 1, 256).is_ok());

  writeBuffers(3);
  // Nothing can be asserted about the current file until the worker has caught up, so only
  // the finished file's share is checked here.
  EXPECT_GE(writer_->getCurrentDuration(), log_.fileDurationSec);
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatOpensTheNextFileWithTheNewFormat) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  auto reprepareResult = writer_->reprepareStreamFormat(44100.0F, 1, 256);
  ASSERT_TRUE(reprepareResult.is_ok());
  EXPECT_EQ(reprepareResult.unwrap(), "session_001.wav");

  const std::vector<std::string> expectedPaths{"session.wav", "session_001.wav"};
  EXPECT_EQ(openedPaths(), expectedPaths);

  std::scoped_lock lock(log_.mutex);
  EXPECT_FLOAT_EQ(log_.lastOpenedFormat.sampleRate, 44100.0F);
  EXPECT_EQ(log_.lastOpenedFormat.channelCount, 1);
  EXPECT_EQ(log_.lastOpenedMaxFramesPerBuffer, 256U);
}

TEST_F(AudioFileWriterTest, ReopenedFilesAreNumberedFromOneAfterThePlainFirstFile) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(48000.0F, 2, 128).is_ok());

  const std::vector<std::string> expectedNames{"session.wav", "session_001.wav", "session_002.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
}

TEST_F(AudioFileWriterTest, RotatedSessionNumbersEveryFile) {
  createWriter(/*rotates=*/true);

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());
  EXPECT_EQ(openResult.unwrap(), "session_001.wav");

  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  const std::vector<std::string> expectedNames{"session_001.wav", "session_002.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatPreservesTheSessionTotals) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  // The file finished by the reprepare plus the final one: the totals must cover both.
  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(std::get<0>(totals), 2.0 * log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(std::get<1>(totals), 2.0 * log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatWithoutOpenFails) {
  createWriter(/*rotates=*/false);
  EXPECT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_err());
}

TEST_F(AudioFileWriterTest, SessionTotalsStartOverWithEachOpen) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());
  ASSERT_TRUE(writer_->closeFile().is_ok());

  ASSERT_TRUE(open().is_ok());
  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(std::get<0>(totals), log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(std::get<1>(totals), log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, RotatesOnceTheFileOutgrowsTheCap) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes + 1);

  writeBuffers(kBuffersBetweenSizeChecks);
  ASSERT_TRUE(writer_->closeFile().is_ok());

  const std::vector<std::string> expectedNames{"session_001.wav", "session_002.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
  EXPECT_EQ(openedPaths(), expectedNames);
}

TEST_F(AudioFileWriterTest, FileSizeIsMeasuredOnlyEveryNthBuffer) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes + 1);

  writeBuffers(kBuffersBetweenSizeChecks - 1);
  ASSERT_TRUE(writer_->closeFile().is_ok());

  EXPECT_EQ(openedFileNames().size(), 1U);
}

TEST_F(AudioFileWriterTest, FileUnderTheCapIsLeftAlone) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes);

  writeBuffers(kBuffersBetweenSizeChecks * 3);
  ASSERT_TRUE(writer_->closeFile().is_ok());

  EXPECT_EQ(openedFileNames().size(), 1U);
}

TEST_F(AudioFileWriterTest, SessionWithoutRotationNeverRotates) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes * 4);

  writeBuffers(kBuffersBetweenSizeChecks * 3);
  ASSERT_TRUE(writer_->closeFile().is_ok());

  EXPECT_EQ(openedFileNames().size(), 1U);
}

TEST_F(AudioFileWriterTest, RotationFoldsFinishedFilesIntoTheTotals) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes + 1);

  writeBuffers(kBuffersBetweenSizeChecks);

  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());
  ASSERT_EQ(openedFileNames().size(), 2U);

  // The rotated file plus the final one.
  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(std::get<0>(totals), 2.0 * log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(std::get<1>(totals), 2.0 * log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, AFailedRotationStopsTheWriter) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes + 1);
  log_.failNextOpen.store(true);

  writeBuffers(kBuffersBetweenSizeChecks);

  EXPECT_TRUE(writer_->closeFile().is_err());
  const std::vector<std::string> expectedPaths{"session_001.wav"};
  EXPECT_EQ(openedPaths(), expectedPaths);
}

// NOLINTEND
