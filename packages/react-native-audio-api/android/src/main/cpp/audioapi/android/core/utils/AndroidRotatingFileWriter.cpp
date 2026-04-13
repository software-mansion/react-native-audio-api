#include <android/log.h>
#include <audioapi/android/core/utils/AndroidRotatingFileWriter.h>

#include <audioapi/core/utils/RotatingFileWriter.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

AndroidRotatingFileWriter::AndroidRotatingFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    size_t rotateIntervalBytes,
    WriterFactory writerFactory,
    std::function<void(const std::string &)> onSegmentFileOpened)
    : AndroidFileWriterBackend(audioEventHandlerRegistry, fileProperties),
      RotatingFileWriter(
          rotateIntervalBytes,
          std::move(writerFactory),
          std::move(onSegmentFileOpened)) {}

OpenFileResult AndroidRotatingFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t streamMaxBufferSizeInFrames,
    const std::string &fileNameOverride) {
  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  streamMaxBufferSize_ = streamMaxBufferSizeInFrames;
  fileProperties_->fileNamePrefix = !fileNameOverride.empty() ? fileNameOverride : "recording";
  if (currentWriter_ == nullptr) {
    currentWriter_ = writerFactory_(fileProperties_);
  }

  return openInnerWriter();
}

CloseFileResult AndroidRotatingFileWriter::closeFile() {
  return RotatingFileWriter::closeFile();
}

void AndroidRotatingFileWriter::writeAudioData(AudioDataType data, int numFrames) {
  if (currentWriter_ == nullptr) {
    return;
  }

  currentWriter_->writeAudioData(data, numFrames);

  writesSinceLastCheck_++;
  if (writesSinceLastCheck_ >= FILE_SIZE_CHECK_WRITE_INTERVAL) {
    writesSinceLastCheck_ = 0;
    size_t size = currentWriter_->getFileSizeBytes();
    if (size > rotateIntervalBytes_) {
      rotateFiles();
    }
  }
  framesWritten_.fetch_add(numFrames, std::memory_order_relaxed);
}

double AndroidRotatingFileWriter::getCurrentDuration() const {
  return static_cast<double>(framesWritten_.load(std::memory_order_relaxed)) /
      fileProperties_->sampleRate;
}

void AndroidRotatingFileWriter::rotateFiles() {
  RotatingFileWriter::rotateFiles();
  openInnerWriter();
}

OpenFileResult AndroidRotatingFileWriter::openInnerWriter() {
  auto inner = std::static_pointer_cast<AndroidFileWriterBackend>(currentWriter_);
  auto result = inner->openFile(streamSampleRate_, streamChannelCount_, streamMaxBufferSize_, "");
  if (result.is_ok() && onSegmentFileOpened_ && currentWriter_ != nullptr) {
    onSegmentFileOpened_(currentWriter_->getFilePath());
  }
  return result;
}

void AndroidRotatingFileWriter::taskOffloaderFunction(WriterData data) {
  auto inner = std::static_pointer_cast<AndroidFileWriterBackend>(currentWriter_);
  inner->taskOffloaderFunction(data);
}
} // namespace audioapi
