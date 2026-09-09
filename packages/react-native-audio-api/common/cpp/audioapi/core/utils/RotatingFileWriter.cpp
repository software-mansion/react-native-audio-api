#include <audioapi/core/utils/RotatingFileWriter.h>

#include <audioapi/core/utils/EncodedAudioFileWriter.h>
#include <audioapi/core/utils/RecordingFileName.h>

#include <memory>
#include <mutex>
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
      onSegmentFileOpened_(std::move(onSegmentFileOpened)),
      rotateIntervalBytes_(rotateIntervalBytes),
      segmentWriter_(std::move(segmentWriter)) {
  // A deliberate downcast rather than an interface: the base stays what the recorder
  // consumes, and a single-file writer would otherwise carry rotation stubs it never uses.
  // Only a writer that can hand off to the next file can be rotated; anything else simply
  // records to the single file it was opened with.
  rotatable_ = dynamic_cast<EncodedAudioFileWriter *>(segmentWriter_.get());
  if (rotatable_ != nullptr) {
    rotatable_->setOnBufferEncodedCallback([this] { onSegmentWriterBufferEncoded(); });
  }
}

OpenFileResult RotatingFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer,
    const std::string &fileNameOverride) {
  {
    std::scoped_lock lock(segmentMutex_);
    sessionStem_ = fileNameOverride;
    segmentIndex_ = 0;
  }
  writesSinceLastCheck_ = 0;
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

  // Closing the segment writer drains and joins its worker, so no rotation can land between
  // here and the totals below. Deliberately outside segmentMutex_, which that worker takes.
  auto closeResult = segmentWriter_->closeFile();
  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }
  foldRetiredSegment(closeResult.unwrap());

  std::scoped_lock lock(segmentMutex_);
  const double totalSizeMB = cumulativeSizeMB_;
  const double totalDurationSec = cumulativeDurationSec_;
  cumulativeSizeMB_ = 0.0;
  cumulativeDurationSec_ = 0.0;
  return CloseFileResult::Ok({totalSizeMB, totalDurationSec});
}

/// @brief Audio thread. Hands the buffer straight to the segment writer: whether the segment
/// is full is decided on that writer's worker thread, not here.
void RotatingFileWriter::writeAudioData(const float *interleavedFrames, int numFrames) {
  if (!isFileOpen()) {
    return;
  }

  segmentWriter_->writeAudioData(interleavedFrames, numFrames);
}

/// @brief Worker thread of the segment writer, called once per encoded buffer. Measuring a
/// file costs a stat on some platforms, so only every Nth buffer is measured.
void RotatingFileWriter::onSegmentWriterBufferEncoded() {
  if (!isFileOpen()) {
    return;
  }

  if (++writesSinceLastCheck_ < FILE_SIZE_CHECK_WRITE_INTERVAL) {
    return;
  }
  writesSinceLastCheck_ = 0;

  if (segmentWriter_->getFileSizeBytes() <= rotateIntervalBytes_) {
    return;
  }

  auto rotated = rotatable_->switchToFile(nextSegmentStem());
  if (rotated.is_err()) {
    isFileOpen_.store(false, std::memory_order_release);
    invokeOnErrorCallback("Failed to start the next recording segment: " + rotated.unwrap_err());
    return;
  }

  foldRetiredSegment(rotated.unwrap());
  announceSegmentOpened();
}

std::string RotatingFileWriter::getFilePath() const {
  return segmentWriter_->getFilePath();
}

double RotatingFileWriter::getCurrentDuration() const {
  std::scoped_lock lock(segmentMutex_);
  return cumulativeDurationSec_ + segmentWriter_->getCurrentDuration();
}

size_t RotatingFileWriter::getFileSizeBytes() const {
  return segmentWriter_->getFileSizeBytes();
}

void RotatingFileWriter::assignOnErrorCallbackId(uint64_t callbackId) {
  AudioFileWriter::assignOnErrorCallbackId(callbackId);
  segmentWriter_->assignOnErrorCallbackId(callbackId);
}

/// @brief JS thread. A format change resizes the segment writer's buffer pool, so this is the
/// one rotation that cannot happen underneath the worker and has to close the writer outright.
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

  auto closeResult = segmentWriter_->closeFile();
  if (closeResult.is_ok()) {
    foldRetiredSegment(closeResult.unwrap());
  }

  writesSinceLastCheck_ = 0;
  auto result = openNextSegment();
  isFileOpen_.store(result.is_ok(), std::memory_order_release);
  return result;
}

std::string RotatingFileWriter::nextSegmentStem() {
  std::scoped_lock lock(segmentMutex_);
  return recordingfilename::segmentStem(sessionStem_, ++segmentIndex_);
}

void RotatingFileWriter::foldRetiredSegment(const std::tuple<double, double> &retired) {
  std::scoped_lock lock(segmentMutex_);
  cumulativeSizeMB_ += std::get<0>(retired);
  cumulativeDurationSec_ += std::get<1>(retired);
}

void RotatingFileWriter::announceSegmentOpened() {
  if (onSegmentFileOpened_) {
    onSegmentFileOpened_(segmentWriter_->getFilePath());
  }
}

OpenFileResult RotatingFileWriter::openNextSegment() {
  auto result = segmentWriter_->openFile(
      streamSampleRate_, streamChannelCount_, maxFramesPerBuffer_, nextSegmentStem());
  if (result.is_ok()) {
    announceSegmentOpened();
  }
  return result;
}

} // namespace audioapi
