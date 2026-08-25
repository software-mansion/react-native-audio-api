#include <audioapi/core/utils/RotatingFileWriter.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace audioapi {

RotatingFileWriter::RotatingFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    size_t rotateIntervalBytes,
    WriterFactory writerFactory,
    OnSegmentFileOpenedCallback onSegmentFileOpened)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties),
      writerFactory_(std::move(writerFactory)),
      onSegmentFileOpened_(std::move(onSegmentFileOpened)),
      rotateIntervalBytes_(rotateIntervalBytes) {}

OpenFileResult RotatingFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer,
    const std::string &fileNameOverride) {
  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;

  if (!fileNameOverride.empty()) {
    fileProperties_->fileNamePrefix = fileNameOverride + fileProperties_->fileNamePrefix;
  }
  if (currentWriter_ == nullptr) {
    currentWriter_ = writerFactory_(fileProperties_);
  }

  return openInnerWriter();
}

CloseFileResult RotatingFileWriter::closeFile() {
  if (currentWriter_ == nullptr) {
    return CloseFileResult::Err("No file open");
  }

  auto closeResult = currentWriter_->closeFile();
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
  if (currentWriter_ == nullptr) {
    return;
  }

  currentWriter_->writeAudioData(interleavedFrames, numFrames);

  writesSinceLastCheck_++;
  if (writesSinceLastCheck_ >= FILE_SIZE_CHECK_WRITE_INTERVAL) {
    writesSinceLastCheck_ = 0;
    if (currentWriter_->getFileSizeBytes() > rotateIntervalBytes_) {
      rotateFiles();
    }
  }
}

std::string RotatingFileWriter::getFilePath() const {
  return currentWriter_ != nullptr ? currentWriter_->getFilePath() : "";
}

double RotatingFileWriter::getCurrentDuration() const {
  double currentSegmentDuration = 0.0;
  if (currentWriter_ != nullptr) {
    currentSegmentDuration = currentWriter_->getCurrentDuration();
  }
  return cumulativeDurationSec_ + currentSegmentDuration;
}

size_t RotatingFileWriter::getFileSizeBytes() const {
  return currentWriter_ != nullptr ? currentWriter_->getFileSizeBytes() : 0;
}

OpenFileResult RotatingFileWriter::reprepareStreamFormat(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (currentWriter_ == nullptr) {
    return OpenFileResult::Err("No file open");
  }

  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;

  return rotateFiles();
}

OpenFileResult RotatingFileWriter::rotateFiles() {
  auto rotatedClose = currentWriter_->closeFile();
  if (rotatedClose.is_ok()) {
    const auto &segment = rotatedClose.unwrap();
    cumulativeSizeMB_ += std::get<0>(segment);
    cumulativeDurationSec_ += std::get<1>(segment);
  }

  return openInnerWriter();
}

OpenFileResult RotatingFileWriter::openInnerWriter() {
  auto result =
      currentWriter_->openFile(streamSampleRate_, streamChannelCount_, maxFramesPerBuffer_, "");
  if (result.is_ok() && onSegmentFileOpened_) {
    onSegmentFileOpened_(currentWriter_->getFilePath());
  }
  return result;
}

} // namespace audioapi
