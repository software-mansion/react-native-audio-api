#include <audioapi/core/utils/RotatingFileWriter.h>

#include <chrono>
#include <memory>
#include <string>

namespace audioapi {

RotatingFileWriter::RotatingFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry>& audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties>& fileProperties,
    size_t rotateIntervalBytes,
    WriterFactory writerFactory)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties),
      writerFactory_(writerFactory),
      rotateIntervalBytes_(rotateIntervalBytes),
      baseFileName_(fileProperties->fileNamePrefix) {}

OpenFileResult RotatingFileWriter::openFile() {
  if (currentWriter_ == nullptr) {
    openNewFile();
  }
  return currentWriter_->openFile();
}

CloseFileResult RotatingFileWriter::closeFile() {
  return currentWriter_ != nullptr ? currentWriter_->closeFile() : CloseFileResult::Err("No file open");
}

std::string RotatingFileWriter::getFilePath() const {
  return currentWriter_ != nullptr ? currentWriter_->getFilePath() : "";
}

bool RotatingFileWriter::writeAudioData(AudioDataType data, int numFrames) {
  if (currentWriter_ == nullptr) {
    return false;
  }

  bool success = currentWriter_->writeAudioData(data, numFrames);

  if (success) {
      writesSinceLastCheck_++;
      // Check file size every ~10 writes to avoid syscall overhead
      if (writesSinceLastCheck_ >= 10) {
          writesSinceLastCheck_ = 0;
          size_t size = currentWriter_->getFileSizeBytes();
          currentFileBytes_ = size;
          if (size > rotateIntervalBytes_) {
              rotateFiles();
          }
      }
      framesWritten_.fetch_add(numFrames, std::memory_order_relaxed);
  }
  return success;
}

double RotatingFileWriter::getCurrentDuration() const {
  return static_cast<double>(framesWritten_.load()) / fileProperties_->sampleRate;
}

size_t RotatingFileWriter::getFileSizeBytes() const {
  return currentWriter_ != nullptr ? currentWriter_->getFileSizeBytes() : 0;
}

void RotatingFileWriter::rotateFiles() {
  if (currentWriter_ != nullptr) {
    currentWriter_->closeFile();
    // Start new file
    openNewFile();
    currentWriter_->openFile();
  }
}

void RotatingFileWriter::openNewFile() {
  auto newProperties = createRotatedProperties();
  currentWriter_ = writerFactory_(newProperties);
  currentFileBytes_ = 0;
}

std::shared_ptr<AudioFileProperties> RotatingFileWriter::createRotatedProperties() {
  auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();

  std::string newName = baseFileName_ + "." + std::to_string(ts);

  auto newProps = std::make_shared<AudioFileProperties>(*fileProperties_);
  newProps->fileNamePrefix = newName;
  return newProps;
}

} // namespace audioapi
