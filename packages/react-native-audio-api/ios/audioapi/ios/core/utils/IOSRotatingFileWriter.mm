#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include "audioapi/core/utils/AudioFileWriter.h"
#include "audioapi/core/utils/RotatingFileWriter.h"

#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/IOSRotatingFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <tuple>
#include <utility>

namespace audioapi {

IOSRotatingFileWriter::IOSRotatingFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
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
  fileProperties_->fileNamePrefix = !fileNameOverride.empty() ? fileNameOverride : "recording";
  if (currentWriter_ == nullptr) {
    currentWriter_ = writerFactory_(fileProperties_);
  }

  return openInnerWriter();
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
  framesWritten_.fetch_add(numFrames, std::memory_order_relaxed);
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
  return static_cast<double>(framesWritten_.load(std::memory_order_relaxed)) /
      fileProperties_->sampleRate;
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
