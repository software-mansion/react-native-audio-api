#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <test/src/utils/FakeFileBackend.h>

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
using namespace audioapi::test_support;

// NOLINTBEGIN

namespace {

// Mirror AudioFileWriter::FILE_SIZE_CHECK_WRITE_INTERVAL and POOL_SIZE.
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
        AudioFileProperties::PathConfig{
            .directory = AudioFileProperties::FileDirectory::Cache,
            .subDirectory = "",
            .fileName = fileName,
        },
        AudioLayout{.sampleRate = 48000.0F, .channelCount = kChannelCount},
        AudioFileProperties::EncodingConfig{
            .format = AudioFileProperties::FileFormat::WAV,
            .bitRate = 128000,
            .bitDepth = AudioFileProperties::BitDepth::Bit16,
            .flacCompressionLevel = 5,
            .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
        },
        AudioFileProperties::WriterConfig{
            .rotateIntervalBytes = rotates ? kRotateIntervalBytes : size_t{0},
            .androidFlushIntervalMs = 0,
        });

    writer_ = std::make_unique<AudioFileWriter>(eventRegistry_, properties_, backend_);
  }

  OpenFileResult open() {
    return writer_->openFile(
        StreamFormat{
            .layout = {.sampleRate = 48000.0F, .channelCount = kChannelCount},
            .maxFramesPerBuffer = kFramesPerBuffer,
        });
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

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry_;
  std::shared_ptr<AudioFileProperties> properties_;
  FakeEncoderLog log_;
  /// Declared before writer_, which borrows it.
  PlatformFileBackend backend_ = makeFakeBackend(log_);
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
  EXPECT_EQ(writer_->getSessionFilePaths(), expectedPaths);
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

TEST_F(AudioFileWriterTest, OpenFailsWhileAFinishedSessionIsNotClosed) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  log_.failNextReprepare.store(true);
  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_err());

  EXPECT_TRUE(open().is_err());

  ASSERT_TRUE(writer_->closeFile().is_ok());
  EXPECT_TRUE(open().is_ok());
}

TEST_F(AudioFileWriterTest, ACloseThatFailsLeavesTheSessionToReportLater) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  log_.failNextClose.store(true);

  EXPECT_TRUE(writer_->closeFile().is_err());

  auto retry = writer_->closeFile();
  ASSERT_TRUE(retry.is_ok());
  EXPECT_EQ(retry.unwrap().filePaths.size(), 1U);
  EXPECT_TRUE(writer_->closeFile().is_err());
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

  auto reprepareResult = writer_->reprepareStreamFormat({44100.0F, 1, 256});
  ASSERT_TRUE(reprepareResult.is_ok());
  EXPECT_EQ(reprepareResult.unwrap(), "session.wav");
  EXPECT_EQ(writer_->getFilePath(), "session.wav");

  const std::vector<std::string> expectedPaths{"session.wav"};
  EXPECT_EQ(writer_->getSessionFilePaths(), expectedPaths);
  EXPECT_EQ(openedFileNames(), expectedPaths);
  EXPECT_EQ(log_.closedFiles.load(), 0);

  std::scoped_lock lock(log_.mutex);
  EXPECT_FLOAT_EQ(log_.lastRepreparedFormat.layout.sampleRate, 44100.0F);
  EXPECT_EQ(log_.lastRepreparedFormat.layout.channelCount, 1);
  EXPECT_EQ(log_.lastRepreparedMaxFramesPerBuffer, 256U);
}

TEST_F(AudioFileWriterTest, RepeatedFormatChangesStayInTheOneFile) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat({48000.0F, 2, 128}).is_ok());

  const std::vector<std::string> expectedNames{"session.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
}

TEST_F(AudioFileWriterTest, BuffersQueuedBeforeAFormatChangeAreEncodedInTheOldFormat) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  writeBuffers(5);
  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_ok());

  // The reprepare joins the worker, so every earlier buffer has been encoded by now.
  EXPECT_EQ(log_.encodedBuffers.load(), 5);
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, DurationCarriesAcrossAFormatChange) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  writeBuffers(3);
  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_ok());

  const double secondsBeforeTheChange = 3.0 * kFramesPerBuffer / 48000.0;
  EXPECT_DOUBLE_EQ(writer_->getCurrentDuration(), secondsBeforeTheChange);
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, RotatedSessionKeepsItsSegmentAcrossAFormatChange) {
  createWriter(/*rotates=*/true);

  auto openResult = open();
  ASSERT_TRUE(openResult.is_ok());
  EXPECT_EQ(openResult.unwrap(), "session_001.wav");

  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_ok());

  const std::vector<std::string> expectedNames{"session_001.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatLeavesOneFileInTheTotals) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_ok());

  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(totals.sizeMB, log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(totals.durationSec, log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatWithoutOpenFails) {
  createWriter(/*rotates=*/false);
  EXPECT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_err());
}

TEST_F(AudioFileWriterTest, ReprepareStreamFormatRejectsAnInvalidFormat) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());

  EXPECT_TRUE(writer_->reprepareStreamFormat({0.0F, 1, 256}).is_err());
  EXPECT_EQ(writer_->getFilePath(), "session.wav");
  ASSERT_TRUE(writer_->closeFile().is_ok());
}

TEST_F(AudioFileWriterTest, AFailedFormatChangeFinishesTheFile) {
  createWriter(/*rotates=*/false);
  ASSERT_TRUE(open().is_ok());
  log_.failNextReprepare.store(true);

  EXPECT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_err());

  EXPECT_EQ(log_.closedFiles.load(), 1);

  // Closing still reports the file, without closing it a second time.
  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());
  EXPECT_EQ(closeResult.unwrap().filePaths.size(), 1U);
  EXPECT_DOUBLE_EQ(closeResult.unwrap().durationSec, log_.fileDurationSec);
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
  ASSERT_TRUE(writer_->reprepareStreamFormat({44100.0F, 1, 256}).is_ok());
  ASSERT_TRUE(writer_->closeFile().is_ok());

  ASSERT_TRUE(open().is_ok());
  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  const auto &totals = closeResult.unwrap();
  EXPECT_DOUBLE_EQ(totals.sizeMB, log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(totals.durationSec, log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, RotatesOnceTheFileOutgrowsTheCap) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes + 1);

  writeBuffers(kBuffersBetweenSizeChecks);
  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());

  const std::vector<std::string> expectedNames{"session_001.wav", "session_002.wav"};
  EXPECT_EQ(openedFileNames(), expectedNames);
  EXPECT_EQ(closeResult.unwrap().filePaths, expectedNames);
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
  EXPECT_DOUBLE_EQ(totals.sizeMB, 2.0 * log_.fileSizeMB);
  EXPECT_DOUBLE_EQ(totals.durationSec, 2.0 * log_.fileDurationSec);
}

TEST_F(AudioFileWriterTest, AFailedRotationStopsTheWriter) {
  createWriter(/*rotates=*/true);
  ASSERT_TRUE(open().is_ok());
  log_.fileSizeBytes.store(kRotateIntervalBytes + 1);
  log_.failNextOpen.store(true);

  writeBuffers(kBuffersBetweenSizeChecks);

  // Closing reports the segment that was finished before the next one failed to open.
  auto closeResult = writer_->closeFile();
  ASSERT_TRUE(closeResult.is_ok());
  const std::vector<std::string> expectedPaths{"session_001.wav"};
  EXPECT_EQ(closeResult.unwrap().filePaths, expectedPaths);
  EXPECT_DOUBLE_EQ(closeResult.unwrap().durationSec, log_.fileDurationSec);
  EXPECT_EQ(log_.closedFiles.load(), 1);
}

// NOLINTEND
