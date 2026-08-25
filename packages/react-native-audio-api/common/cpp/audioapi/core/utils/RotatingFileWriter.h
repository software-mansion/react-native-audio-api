#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace audioapi {

/// Splits a recording into size-capped segments. Owns no encoder of its own: every
/// segment is a writer built by @p writerFactory, and this class only decides when to
/// close one and open the next, accumulating size and duration across them.
class RotatingFileWriter final : public AudioFileWriter {
 public:
  using WriterFactory =
      std::function<std::shared_ptr<AudioFileWriter>(const std::shared_ptr<AudioFileProperties> &)>;
  using OnSegmentFileOpenedCallback = std::function<void(const std::string &)>;

  RotatingFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      size_t rotateIntervalBytes,
      WriterFactory writerFactory,
      OnSegmentFileOpenedCallback onSegmentFileOpened = {});

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer,
      const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

  void writeAudioData(const float *interleavedFrames, int numFrames) override;

  /// Closes the current segment like a rotation — folding its size and duration into the
  /// cumulative totals — and opens the next one with the new stream format.
  OpenFileResult reprepareStreamFormat(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer);

  [[nodiscard]] std::string getFilePath() const override;
  [[nodiscard]] double getCurrentDuration() const override;
  [[nodiscard]] size_t getFileSizeBytes() const override;

 private:
  /// Checking the file size costs a stat(), so only do it every Nth write.
  static constexpr int FILE_SIZE_CHECK_WRITE_INTERVAL = 10;

  OpenFileResult rotateFiles();
  OpenFileResult openInnerWriter();

  WriterFactory writerFactory_;
  OnSegmentFileOpenedCallback onSegmentFileOpened_;
  size_t rotateIntervalBytes_;
  size_t writesSinceLastCheck_ = 0;
  std::shared_ptr<AudioFileWriter> currentWriter_;

  double cumulativeSizeMB_{0.0};
  double cumulativeDurationSec_{0.0};

  float streamSampleRate_{0.0F};
  int32_t streamChannelCount_{0};
  int32_t maxFramesPerBuffer_{0};
};

} // namespace audioapi
