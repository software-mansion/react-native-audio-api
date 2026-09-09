#include <audioapi/core/utils/RotatingFileWriter.h>

#include <audioapi/core/utils/RecordingFileName.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace audioapi {

RotatingFileWriter::RotatingFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    size_t rotateIntervalBytes,
    std::shared_ptr<AudioFileWriter> segmentWriter,
    OnSegmentFileOpenedCallback onSegmentFileOpened)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties),
      segmentWriter_(std::move(segmentWriter)),
      onSegmentFileOpened_(std::move(onSegmentFileOpened)),
      rotateIntervalBytes_(rotateIntervalBytes) {}

OpenFileResult RotatingFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer,
    const std::string &fileNameOverride) {
  sessionStem_ = fileNameOverride;
  segmentIndex_ = 0;
  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;

  auto result = openNextSegment();
  isFileOpen_.store(result.is_ok(), std::memory_order_release);
  return result;
}

CloseFileResult RotatingFileWriter::closeFile() {
  if (!isFileOpen()) {
    return CloseFileResult::Err("No file open");
  }
  isFileOpen_.store(false, std::memory_order_release);

  auto closeResult = segmentWriter_->closeFile();
  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }

  const auto &lastSegment = closeResult.unwrap();
  const double totalSizeMB = cumulativeSizeMB_ + std::get<0>(lastSegment);
  const double totalDurationSec = cumulativeDurationSec_ + std::get<1>(lastSegment);
  cumulativeSizeMB_ = 0.0;
  cumulativeDurationSec_ = 0.0;
  return CloseFileResult::Ok({totalSizeMB, totalDurationSec});
}

void RotatingFileWriter::writeAudioData(const float *interleavedFrames, int numFrames) {
  if (!isFileOpen()) {
    return;
  }

  segmentWriter_->writeAudioData(interleavedFrames, numFrames);

  writesSinceLastCheck_++;
  if (writesSinceLastCheck_ >= FILE_SIZE_CHECK_WRITE_INTERVAL) {
    writesSinceLastCheck_ = 0;
    if (segmentWriter_->getFileSizeBytes() > rotateIntervalBytes_) {
      rotateFiles();
    }
  }
}

std::string RotatingFileWriter::getFilePath() const {
  return segmentWriter_->getFilePath();
}

double RotatingFileWriter::getCurrentDuration() const {
  return cumulativeDurationSec_ + segmentWriter_->getCurrentDuration();
}

size_t RotatingFileWriter::getFileSizeBytes() const {
  return segmentWriter_->getFileSizeBytes();
}

OpenFileResult RotatingFileWriter::reprepareStreamFormat(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (!isFileOpen()) {
    return OpenFileResult::Err("No file open");
  }

  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;

  return rotateFiles();
}

OpenFileResult RotatingFileWriter::rotateFiles() {
  auto rotatedClose = segmentWriter_->closeFile();
  if (rotatedClose.is_ok()) {
    const auto &segment = rotatedClose.unwrap();
    cumulativeSizeMB_ += std::get<0>(segment);
    cumulativeDurationSec_ += std::get<1>(segment);
  }

  return openNextSegment();
}

OpenFileResult RotatingFileWriter::openNextSegment() {
  auto result = segmentWriter_->openFile(
      streamSampleRate_,
      streamChannelCount_,
      maxFramesPerBuffer_,
      recordingfilename::segmentStem(sessionStem_, ++segmentIndex_));
  if (result.is_ok() && onSegmentFileOpened_) {
    onSegmentFileOpened_(segmentWriter_->getFilePath());
  }
  return result;
}

} // namespace audioapi
