#include <audioapi/AndroidEncoding.h>
#include <audioapi/android/core/utils/FileOptions.h>
#include <audioapi/android/core/utils/OsAudioFileWriter.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <sys/stat.h>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

OsAudioFileWriter::OsAudioFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t streamMaxBufferSize)
    : AndroidFileWriterBackend(audioEventHandlerRegistry, fileProperties) {}

OsAudioFileWriter::~OsAudioFileWriter() {
  isFileOpen_.store(false, std::memory_order_release);
  cleanupPreallocatedInputPool();
  encoder_.reset();
}

OpenFileResult OsAudioFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t streamMaxBufferSize,
    const std::string &fileNameOverride) {
  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  streamMaxBufferSize_ = streamMaxBufferSize;
  framesWritten_.store(0, std::memory_order_release);

  if (fileProperties_->sampleRate <= 0 || fileProperties_->channelCount <= 0) {
    return OpenFileResult::Err(
        "Invalid file properties: sampleRate and channelCount must be greater than 0");
  }

  auto specResult = EncoderCapabilities::resolveOutputSpec(fileProperties_->format);
  if (specResult.is_err()) {
    return OpenFileResult::Err(specResult.unwrap_err());
  }
  EncoderOutputSpec outputSpec = specResult.unwrap();

  auto filePathResult = android::fileoptions::getFilePath(fileProperties_, fileNameOverride);
  if (!filePathResult.is_ok()) {
    return OpenFileResult::Err(filePathResult.unwrap_err());
  }
  filePath_ = filePathResult.unwrap();

  StreamFormat inputFormat{
      .sampleRate = streamSampleRate,
      .channelCount = streamChannelCount,
      .isInterleaved = true,
  };

  auto encoder = std::make_unique<android_encoder::AndroidEncoder>(fileProperties_);
  auto openResult =
      encoder->open(inputFormat, outputSpec, static_cast<size_t>(streamMaxBufferSize), filePath_);
  if (openResult.is_err()) {
    return OpenFileResult::Err(openResult.unwrap_err());
  }
  encoder_ = std::move(encoder);

  if (!initializePreallocatedInputPool()) {
    rollbackFailedOpen();
    return OpenFileResult::Err("Failed to preallocate Android file writer buffers");
  }

  isFileOpen_.store(true, std::memory_order_release);
  return OpenFileResult::Ok(filePath_);
}

void OsAudioFileWriter::rollbackFailedOpen() {
  cleanupPreallocatedInputPool();
  if (encoder_ != nullptr) {
    encoder_->close();
    encoder_.reset();
  }
  if (!filePath_.empty()) {
    std::remove(filePath_.c_str());
    filePath_ = "";
  }
  framesWritten_.store(0, std::memory_order_release);
  isFileOpen_.store(false, std::memory_order_release);
}

CloseFileResult OsAudioFileWriter::closeFile() {
  if (!isFileOpen() || encoder_ == nullptr) {
    return CloseFileResult::Err("File is not open");
  }

  isFileOpen_.store(false, std::memory_order_release);

  // Drains the worker queue before finalizing the encoder below.
  cleanupPreallocatedInputPool();

  auto closeResult = encoder_->close();
  encoder_.reset();
  framesWritten_.store(0, std::memory_order_release);
  filePath_ = "";

  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }
  return CloseFileResult::Ok(closeResult.unwrap());
}

size_t OsAudioFileWriter::getFileSizeBytes() const {
  if (encoder_ != nullptr) {
    return encoder_->getFileSizeBytes();
  }
  struct stat st{};
  if (!filePath_.empty() && stat(filePath_.c_str(), &st) == 0) {
    return static_cast<size_t>(st.st_size);
  }
  return 0;
}

void OsAudioFileWriter::processWriterData(void *audioData, int numFrames) {
  if (!isFileOpen() || encoder_ == nullptr) {
    return;
  }

  auto result = encoder_->encode(audioData, numFrames);
  if (result.is_err()) {
    invokeOnErrorCallback(
        "Failed to write audio data to file: " + filePath_ + " - " + result.unwrap_err());
    return;
  }

  framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
}

} // namespace audioapi
