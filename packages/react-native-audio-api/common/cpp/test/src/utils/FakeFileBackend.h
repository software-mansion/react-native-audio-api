#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <atomic>
#include <cstddef>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// NOLINTBEGIN

namespace audioapi::test_support {

/// Shared by every encoder a session opens. Written on the worker thread; read by the test
/// after closeFile(), which joins that worker.
struct FakeEncoderLog {
  std::atomic<size_t> fileSizeBytes{0};
  std::atomic<int> encodedBuffers{0};
  std::atomic<int> closedFiles{0};
  std::atomic<bool> failNextOpen{false};
  std::atomic<bool> failNextReprepare{false};
  std::atomic<bool> failNextClose{false};
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
  FakeEncoder(const EncoderSettings &settings, FakeEncoderLog &log)
      : AudioEncoder(settings), log_(log) {}

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec & /*outputSpec*/,
      const std::string &filePath) override {
    if (log_.failNextOpen.exchange(false)) {
      return OpenEncoderResult::Err("encoder refused to open");
    }
    inputFormat_ = inputFormat;
    filePath_ = filePath;
    {
      std::scoped_lock lock(log_.mutex);
      log_.lastOpenedFormat = inputFormat;
      log_.lastOpenedMaxFramesPerBuffer = inputFormat.maxFramesPerBuffer;
    }
    markOpen();
    return OpenEncoderResult::Ok(filePath);
  }

  /// Mirrors IOSEncoder::reprepareInput.
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) override {
    if (!isOpen()) {
      return OpenEncoderResult::Err("encoder is not open");
    }
    if (log_.failNextReprepare.exchange(false)) {
      return OpenEncoderResult::Err("encoder refused the new input format");
    }
    inputFormat_ = inputFormat;
    {
      std::scoped_lock lock(log_.mutex);
      log_.lastRepreparedFormat = inputFormat;
      log_.lastRepreparedMaxFramesPerBuffer = inputFormat.maxFramesPerBuffer;
    }
    return OpenEncoderResult::Ok(filePath_);
  }

  EncodeResult encode(const AudioBuffer & /*input*/, int numFrames) override {
    addEncodedFrames(static_cast<size_t>(numFrames));
    log_.encodedBuffers.fetch_add(1, std::memory_order_acq_rel);
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    markClosed();
    log_.closedFiles.fetch_add(1, std::memory_order_acq_rel);
    if (log_.failNextClose.exchange(false)) {
      return CloseEncoderResult::Err("encoder failed to finalize the file");
    }
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
inline PlatformFileBackend makeFakeBackend(FakeEncoderLog &log) {
  return PlatformFileBackend{
      .resolveOutputSpec =
          [&log](AudioFileProperties::FileFormat /*format*/) {
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
      .createEncoder = [&log](const EncoderSettings &settings) -> CreateEncoderResult {
        return CreateEncoderResult::Ok(std::make_unique<FakeEncoder>(settings, log));
      },
  };
}

/// WAV properties for a recording in the cache directory; the fake backend never writes them.
inline std::shared_ptr<AudioFileProperties> wavFileProperties(int channelCount = 1) {
  return std::make_shared<AudioFileProperties>(
      AudioFileProperties::PathConfig{
          .directory = AudioFileProperties::FileDirectory::Cache,
          .subDirectory = "",
          .fileName = "",
      },
      AudioLayout{.sampleRate = 48000.0F, .channelCount = channelCount},
      AudioFileProperties::EncodingConfig{
          .format = AudioFileProperties::FileFormat::WAV,
          .bitRate = 0,
          .bitDepth = AudioFileProperties::BitDepth::Bit16,
          .flacCompressionLevel = 0,
          .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
      },
      AudioFileProperties::WriterConfig{.rotateIntervalBytes = 0, .androidFlushIntervalMs = 0});
}

} // namespace audioapi::test_support

// NOLINTEND
