#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/RotatingFileWriter.h>

#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/IOSRotatingFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <tuple>
#include <utility>

namespace audioapi {

IOSRotatingFileWriter::IOSRotatingFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    size_t rotateIntervalBytes,
    WriterFactory writerFactory,
    std::function<void(const std::string &)> onSegmentFileOpened)
    : IOSFileWriter(audioEventHandlerRegistry, fileProperties),
      RotatingFileWriter(
          rotateIntervalBytes,
          std::move(writerFactory),
          std::move(onSegmentFileOpened)),
      fileProperties_(fileProperties)
{
}

OpenFileResult IOSRotatingFileWriter::openFile(
    AVAudioFormat *streamFormat,
    size_t streamMaxBufferSizeInFrames,
    const std::string &fileNameOverride)
{
  streamFormat_ = streamFormat;
  streamMaxBufferSizeInFrames_ = streamMaxBufferSizeInFrames;
  if (!fileNameOverride.empty()) {
    fileProperties_->fileNamePrefix = fileNameOverride + fileProperties_->fileNamePrefix;
  }
  if (currentWriter_ == nullptr) {
    currentWriter_ = writerFactory_(fileProperties_);
  }

  return openInnerWriter();
}

OpenFileResult IOSRotatingFileWriter::reprepareStreamFormat(
    AVAudioFormat *streamFormat,
    size_t streamMaxBufferSizeInFrames)
{
  streamFormat_ = streamFormat;
  streamMaxBufferSizeInFrames_ = streamMaxBufferSizeInFrames;

  if (currentWriter_ == nullptr) {
    currentWriter_ = writerFactory_(fileProperties_);
    return openInnerWriter();
  }

  rotateFiles();

  if (currentWriter_ == nullptr) {
    return OpenFileResult::Err("Failed to reopen file for writing after input format change");
  }

  return OpenFileResult::Ok(currentWriter_->getFilePath());
}

void IOSRotatingFileWriter::writeAudioData(AudioDataType data, int numFrames)
{
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
}

CloseFileResult IOSRotatingFileWriter::closeFile()
{
  return RotatingFileWriter::closeFile();
}

void IOSRotatingFileWriter::rotateFiles()
{
  RotatingFileWriter::rotateFiles();
  openInnerWriter();
}

double IOSRotatingFileWriter::getCurrentDuration() const
{
  double currentSegmentDuration = 0.0;
  if (currentWriter_ != nullptr) {
    currentSegmentDuration = currentWriter_->getCurrentDuration();
  }
  return cumulativeDurationSec_ + currentSegmentDuration;
}

OpenFileResult IOSRotatingFileWriter::openInnerWriter()
{
  auto inner = std::static_pointer_cast<IOSFileWriter>(currentWriter_);
  auto result = inner->openFile(streamFormat_, streamMaxBufferSizeInFrames_, "");
  if (result.is_ok() && onSegmentFileOpened_ && currentWriter_ != nullptr) {
    onSegmentFileOpened_(currentWriter_->getFilePath());
  }
  return result;
}

} // namespace audioapi
