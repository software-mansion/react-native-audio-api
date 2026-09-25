#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <fstream>
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
  std::atomic<int> closedFiles{0};
  std::atomic<bool> failNextOpen{false};
  std::atomic<bool> failNextReprepare{false};
  /// Creates the first resolved path on disk before returning it, so the writer finds it
  /// taken whatever name it generated.
  std::atomic<bool> occupyFirstResolvedPath{false};
  double fileSizeMB = 1.0;
  double fileDurationSec = 2.0;

  std::mutex mutex;
  /// Resolved paths are this prefix plus the bare file name; empty by default so a test can
  /// assert on the names directly.
  std::string pathPrefix;
  std::vector<std::string> openedFileNames;
  std::vector<std::string> openedPaths;
  StreamFormat lastOpenedFormat{};
  size_t lastOpenedMaxFramesPerBuffer = 0;
  StreamFormat lastRepreparedFormat{};
  size_t lastRepreparedMaxFramesPerBuffer = 0;
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

  /// Mirrors IOSEncoder::reprepareInput, reached through the backend hook.
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat, size_t maxBufferSizeInFrames) {
    if (!isOpen()) {
      return OpenEncoderResult::Err("encoder is not open");
    }
    if (log_.failNextReprepare.exchange(false)) {
      return OpenEncoderResult::Err("encoder refused the new input format");
    }
    inputFormat_ = inputFormat;
    maxBufferSizeInFrames_ = maxBufferSizeInFrames;
    {
      std::scoped_lock lock(log_.mutex);
      log_.lastRepreparedFormat = inputFormat;
      log_.lastRepreparedMaxFramesPerBuffer = maxBufferSizeInFrames;
    }
    return OpenEncoderResult::Ok(filePath_);
  }

  EncodeResult encode(const float *const * /*channels*/, int numFrames) override {
    addEncodedFrames(static_cast<size_t>(numFrames));
    log_.encodedBuffers.fetch_add(1, std::memory_order_acq_rel);
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    markClosed();
    log_.closedFiles.fetch_add(1, std::memory_order_acq_rel);
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
            const std::string path = log.pathPrefix + fileName;
            if (log.occupyFirstResolvedPath.exchange(false)) {
              std::ofstream(path).put('\0');
            }
            return Result<std::string, std::string>::Ok(path);
          },
      .createEncoder = [&log](const std::shared_ptr<AudioFileProperties> &properties)
          -> std::unique_ptr<AudioEncoder> {
        return std::make_unique<FakeEncoder>(properties, log);
      },
      .reprepareEncoderInput =
          [](AudioEncoder &encoder, const StreamFormat &inputFormat, size_t maxFramesPerBuffer) {
            return static_cast<FakeEncoder &>(encoder).reprepareInput(
                inputFormat, maxFramesPerBuffer);
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

  void TearDown() override {
    writer_.reset();
    if (!scratchDir_.empty()) {
      std::filesystem::remove_all(scratchDir_);
    }
  }

  /// Points the fake backend at a directory of its own, for tests that need files on disk.
  void resolvePathsIntoScratchDir() {
    const auto *testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    scratchDir_ = std::filesystem::temp_directory_path() /
        (std::string("AudioFileWriterTest.") + testInfo->name());
    std::filesystem::remove_all(scratchDir_);
    std::filesystem::create_directories(scratchDir_);

    std::scoped_lock lock(log_.mutex);
    log_.pathPrefix = scratchDir_.string() + "/";
  }

  void createWriter(bool rotates, const std::string &fileName = "session") {
    properties_ = std::make_shared<AudioFileProperties>(
        AudioFileProperties::FileDirectory::Cache,
        "",
        fileName,
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
    const float *channels[kChannelCount] = {frames_.data(), frames_.data() + kFramesPerBuffer};
    for (int buffer = 0; buffer < count; ++buffer) {
      writer_->writeAudioData(channels, kFramesPerBuffer);
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
  std::filesystem::path scratchDir_;
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

TEST_F(AudioFileWriterTest, ReprepareStreamFormatKeepsTheFileAndRetargetsTheEncoder) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  auto reprepareResult = writer_->reprepareStreamFormat(44100.0F, 1, 256);
  ASSERT_TRUE(reprepareResult.is_ok());
  EXPECT_EQ(reprepareResult.unwrap(), "session.wav");
  EXPECT_EQ(writer_->getFilePath(), "session.wav");

  const std::vector<std::string> expectedPaths{"session.wav"};
  EXPECT_EQ(openedPaths(), expectedPaths);
  EXPECT_EQ(openedFileNames(), expectedPaths);
  EXPECT_EQ(log_.closedFiles.load(), 0);

  std::scoped_lock lock(log_.mutex);
  EXPECT_FLOAT_EQ(log_.lastRepreparedFormat.sampleRate, 44100.0F);
  EXPECT_EQ(log_.lastRepreparedFormat.channelCount, 1);
  EXPECT_EQ(log_.lastRepreparedMaxFramesPerBuffer, 256U);
}

TEST_F(AudioFileWriterTest, RepeatedFormatChangesStayInTheOneFile) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(48000.0F, 2, 128).is_ok());

  const std::vector<std::string> expectedNames{"session.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
}

TEST_F(AudioFileWriterTest, BuffersQueuedBeforeAFormatChangeAreEncodedInTheOldFormat) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  writeBuffers(5);
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  // The reprepare joins the worker, so every earlier buffer has been encoded by now.
  EXPECT_EQ(log_.encodedBuffers.load(), 5);
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, DurationCarriesAcrossAFormatChange) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  writeBuffers(3);
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  const double secondsBeforeTheChange = 3.0 * kFramesPerBuffer / 48000.0;
  EXPECT_DOUBLE_EQ(writer_->getCurrentDuration(), secondsBeforeTheChange);
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, RotatedSessionKeepsItsSegmentAcrossAFormatChange) {
  createWriter(/*rotates=*/true);

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());
  EXPECT_EQ(openResult.unwrap(), "session_001.wav");

  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  const std::vector<std::string> expectedNames{"session_001.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatLeavesOneFileInTheTotals) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_ok());

  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(std::get<0>(totals), log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(std::get<1>(totals), log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatWithoutOpenFails) {
  createWriter(/*rotates=*/false);
  EXPECT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_err());
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatRejectsAnInvalidFormat) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  EXPECT_TRUE(writer_->reprepareStreamFormat(0.0F, 1, 256).is_err());
  EXPECT_EQ(writer_->getFilePath(), "session.wav");
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, AFailedFormatChangeFinishesTheFile) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  log_.failNextReprepare.store(true);

  EXPECT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_err());

  EXPECT_EQ(log_.closedFiles.load(), 1);
  EXPECT_TRUE(writer_->closeFile().is_err());
}

TEST_F(AudioFileWriterTest, AFormatChangeFailsWithoutAPlatformHookForIt) {
  createWriter(/*rotates=*/false);
  auto backend = makeFakeBackend(log_);
  backend.reprepareEncoderInput = nullptr;
  writer_ = std::make_unique<AudioFileWriter>(eventRegistry_, properties_, nullptr, backend);
  ASSERT_TRUE(open().is_ok());

  EXPECT_TRUE(writer_->reprepareStreamFormat(44100.0F, 1, 256).is_err());
  EXPECT_EQ(log_.closedFiles.load(), 1);
}

TEST_F(AudioFileWriterTest, GeneratedNameStepsAsideForAnExistingFile) {
  resolvePathsIntoScratchDir();
  createWriter(/*rotates=*/false, /*fileName=*/"");
  log_.occupyFirstResolvedPath.store(true);

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());

  const auto names = openedFileNames();
  ASSERT_EQ(names.size(), 2U);
  const std::string stem = names[0].substr(0, names[0].size() - std::string(".wav").size());
  EXPECT_EQ(names[1], stem + "_1.wav");
  EXPECT_EQ(openResult.unwrap(), log_.pathPrefix + names[1]);
}

TEST_F(AudioFileWriterTest, EveryTakenSuffixIsSkipped) {
  resolvePathsIntoScratchDir();
  createWriter(/*rotates=*/false, /*fileName=*/"");
  log_.occupyFirstResolvedPath.store(true);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->closeFile().is_ok());

  // The first session took `<stem>_1`; occupy that too, so the next one has to go past it.
  const auto firstSessionNames = openedFileNames();
  ASSERT_EQ(firstSessionNames.size(), 2U);
  std::ofstream(log_.pathPrefix + firstSessionNames[1]).put('\0');
  const std::string stem =
      firstSessionNames[0].substr(0, firstSessionNames[0].size() - std::string(".wav").size());

  // Only deterministic while the clock stays within the same second; otherwise the new
  // session gets a fresh stem and nothing collides, which is fine too.
  ASSERT_TRUE(open().is_ok());
  const std::string lastName = openedFileNames().back();
  EXPECT_TRUE(lastName == stem + "_2.wav" || lastName.find(stem) == std::string::npos) << lastName;
}

TEST_F(AudioFileWriterTest, RotatedUserNamedSegmentsOverwriteExistingFiles) {
  resolvePathsIntoScratchDir();
  createWriter(/*rotates=*/true);
  std::ofstream(log_.pathPrefix + "session_001.wav").put('\0');

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());

  const std::vector<std::string> expectedNames{"session_001.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
  EXPECT_EQ(openResult.unwrap(), log_.pathPrefix + "session_001.wav");
}

TEST_F(AudioFileWriterTest, UserNamedSingleFileOverwritesAnExistingFile) {
  resolvePathsIntoScratchDir();
  createWriter(/*rotates=*/false);
  std::ofstream(log_.pathPrefix + "session.wav").put('\0');

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());

  const std::vector<std::string> expectedNames{"session.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
  EXPECT_EQ(openResult.unwrap(), log_.pathPrefix + "session.wav");
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
