#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/EncoderFactory.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/FileSystem.hpp>

#ifdef ANDROID
#include <android/log.h>
#endif

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace audioapi {

const PlatformFileBackend &osFileBackend() {
  static const PlatformFileBackend backend{
      .resolveOutputSpec = &encoder_capabilities::resolveOutputSpec,
      .resolvePath = &resolveOsFilePath,
      .createEncoder = &encoding::createEncoder,
  };
  return backend;
}

AudioFileWriter::AudioFileWriter(
    std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    const PlatformFileBackend &backend)
    : fileProperties_(fileProperties), backend_(backend), errorEvent_(std::move(errorEvent)) {}

AudioFileWriter::~AudioFileWriter() {
  isFileOpen_.store(false, std::memory_order_release);
  worker_.stop();
  currentEncoder_.reset();
}

Result<NoneType, std::string> AudioFileWriter::prepare(const StreamFormat &streamFormat) {
  {
    std::scoped_lock lock(fileMutex_);
    if (isSessionOpen_) {
      return Err("file already open");
    }

    sessionStem_ = recording_file_name::sessionStem(fileProperties_);
    sessionFilePaths_.clear();
    openedFileCount_ = 0;
    finishedFilesSizeMB_ = 0.0;
    finishedFilesDurationSec_ = 0.0;
  }

  auto opened = startNextFile(streamFormat);
  if (opened.is_err()) {
    return Err(opened.unwrap_err());
  }
  std::scoped_lock lock(fileMutex_);
  isSessionOpen_ = true;
  return Ok(None);
}

CloseFileResult AudioFileWriter::closeFile() {
  {
    std::scoped_lock lock(fileMutex_);
    if (!isSessionOpen_) {
      return CloseFileResult::Err("file is not open");
    }
  }

  auto finished = finishCurrentFile();
  if (finished.is_err()) {
    return CloseFileResult::Err(finished.unwrap_err());
  }

  std::scoped_lock lock(fileMutex_);
  isSessionOpen_ = false;
  ClosedSession session{
      .filePaths = std::move(sessionFilePaths_),
      .sizeMB = finishedFilesSizeMB_,
      .durationSec = finishedFilesDurationSec_,
  };
  sessionFilePaths_.clear();
  finishedFilesSizeMB_ = 0.0;
  finishedFilesDurationSec_ = 0.0;
  return CloseFileResult::Ok(std::move(session));
}

void AudioFileWriter::close() {
  (void)closeFile();
}

Result<NoneType, std::string> AudioFileWriter::reprepare(const StreamFormat &streamFormat) {
  if (!isFileOpen()) {
    return Err("file is not open");
  }
  if (streamFormat.layout.sampleRate <= 0 || streamFormat.layout.channelCount <= 0 ||
      streamFormat.maxFramesPerBuffer == 0) {
    return Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  worker_.stop();
  isFileOpen_.store(false, std::memory_order_release);

  OpenFileResult reprepareResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    streamFormat_ = streamFormat;
    reprepareResult = changeCurrentFileInputFormat();
  }
  if (reprepareResult.is_err()) {
    // Nothing more can be encoded in the new format, so the file is finished as far as it got.
    finishCurrentFile();
    return Err(reprepareResult.unwrap_err());
  }

  if (!worker_.start(streamFormat_)) {
    finishCurrentFile();
    return Err("Failed to preallocate file writer buffers");
  }

  isFileOpen_.store(true, std::memory_order_release);
  return Ok(None);
}

OpenFileResult AudioFileWriter::startNextFile(const StreamFormat &streamFormat) {
  if (streamFormat.layout.sampleRate <= 0 || streamFormat.layout.channelCount <= 0 ||
      streamFormat.maxFramesPerBuffer == 0) {
    return OpenFileResult::Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  streamFormat_ = streamFormat;

  OpenFileResult openResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    openResult = openNextFile();
  }
  if (openResult.is_err()) {
    return openResult;
  }

  if (!worker_.start(streamFormat_)) {
    rollbackFailedOpen();
    return OpenFileResult::Err("Failed to preallocate file writer buffers");
  }

  {
    std::scoped_lock lock(fileMutex_);
    sessionFilePaths_.push_back(openResult.unwrap());
  }
  isFileOpen_.store(true, std::memory_order_release);
  return openResult;
}

CloseEncoderResult AudioFileWriter::finishCurrentFile() {
  // Drains and joins the worker while the file still counts as open, so buffers already
  // queued are encoded rather than dropped. Must run without fileMutex_: the worker takes
  // it, and joining while holding it would deadlock.
  worker_.stop();
  isFileOpen_.store(false, std::memory_order_release);

  std::scoped_lock lock(fileMutex_);
  // Already finished by a failed format change, or by a rotation that could not open the next
  // file, possibly one the drain above just ran.
  if (currentEncoder_ == nullptr) {
    return CloseEncoderResult::Ok({0.0, 0.0});
  }

  auto closeResult = closeCurrentFile();

  if (closeResult.is_ok()) {
    addFinishedFile(closeResult.unwrap());
  }
  return closeResult;
}

std::string AudioFileWriter::fileStem(size_t fileNumber) const {
  if (rotatesFiles()) {
    return recording_file_name::segmentStem(sessionStem_, fileNumber);
  }
  return sessionStem_;
}

static void warnAboutOverwrite(const std::string &path) {
#ifdef ANDROID
  __android_log_print(
      ANDROID_LOG_WARN, "RN_AUDIOAPI", "recording overwrites an existing file: %s", path.c_str());
#else
  printf("[RN_AUDIOAPI WARN] recording overwrites an existing file: %s\n", path.c_str());
#endif
}

Result<std::string, std::string> AudioFileWriter::resolveNextFilePath(
    const std::string &stem,
    const std::string &extension) const {
  auto pathResult = backend_.resolvePath(fileProperties_, stem + "." + extension);
  if (pathResult.is_err()) {
    return pathResult;
  }

  const bool userNamed = !fileProperties_->path.fileName.empty();
  if (userNamed) {
    if (file_system::fileExists(pathResult.unwrap())) {
      warnAboutOverwrite(pathResult.unwrap());
    }
    return pathResult;
  }

  return firstUnusedPath(stem, extension, std::move(pathResult).unwrap());
}

Result<std::string, std::string> AudioFileWriter::firstUnusedPath(
    const std::string &stem,
    const std::string &extension,
    std::string path) const {
  for (size_t suffix = 1; file_system::fileExists(path); ++suffix) {
    auto suffixedPath = backend_.resolvePath(
        fileProperties_, stem + "_" + std::to_string(suffix) + "." + extension);
    if (suffixedPath.is_err()) {
      return suffixedPath;
    }
    path = std::move(suffixedPath).unwrap();
  }
  return Ok(std::move(path));
}

OpenFileResult AudioFileWriter::openNextFile() {
  // Calling an empty std::function throws, which on the worker thread would terminate.
  if (!backend_.resolveOutputSpec || !backend_.resolvePath || !backend_.createEncoder) {
    return OpenFileResult::Err("File writer was constructed without a platform backend");
  }

  auto specResult = backend_.resolveOutputSpec(fileProperties_->encoding.format);
  if (specResult.is_err()) {
    return OpenFileResult::Err(specResult.unwrap_err());
  }
  const auto &outputSpec = specResult.unwrap();

  auto filePathResult =
      resolveNextFilePath(fileStem(openedFileCount_ + 1), std::string(outputSpec.extension));
  if (filePathResult.is_err()) {
    return OpenFileResult::Err(filePathResult.unwrap_err());
  }
  const std::string &filePath = filePathResult.unwrap();

  auto encoderResult = backend_.createEncoder(
      EncoderSettings{
          .fileLayout = fileProperties_->fileLayout,
          .encoding = fileProperties_->encoding,
      });
  if (encoderResult.is_err()) {
    return OpenFileResult::Err(encoderResult.unwrap_err());
  }
  auto encoder = std::move(encoderResult).unwrap();
  auto openResult = encoder->open(streamFormat_, outputSpec, filePath);
  if (openResult.is_err()) {
    return openResult;
  }

  currentEncoder_ = std::move(encoder);
  ++openedFileCount_;
  writesSinceLastSizeCheck_ = 0;
  return OpenFileResult::Ok(currentEncoder_->getFilePath());
}

OpenFileResult AudioFileWriter::changeCurrentFileInputFormat() {
  if (currentEncoder_ == nullptr) {
    return OpenFileResult::Err("file is not open");
  }

  auto result = currentEncoder_->changeInputFormat(streamFormat_);
  if (result.is_err()) {
    return OpenFileResult::Err(
        "Failed to switch the recording to the new input format: " + result.unwrap_err());
  }
  return OpenFileResult::Ok(currentEncoder_->getFilePath());
}

CloseEncoderResult AudioFileWriter::closeCurrentFile() {
  if (currentEncoder_ == nullptr) {
    return CloseEncoderResult::Err("file is not open");
  }

  auto closeResult = currentEncoder_->close();
  currentEncoder_.reset();
  return closeResult;
}

void AudioFileWriter::addFinishedFile(const std::tuple<double, double> &finished) {
  finishedFilesSizeMB_ += std::get<0>(finished);
  finishedFilesDurationSec_ += std::get<1>(finished);
}

void AudioFileWriter::rollbackFailedOpen() {
  worker_.stop();

  std::scoped_lock lock(fileMutex_);
  if (currentEncoder_ != nullptr) {
    // Whatever the encoder reports about a file that is about to be deleted is of no use.
    (void)currentEncoder_->close();
    file_system::removeFile(currentEncoder_->getFilePath());
    currentEncoder_.reset();
  }
  isFileOpen_.store(false, std::memory_order_release);
}

void AudioFileWriter::rotateIfFileOutgrowsCap() {
  if (!rotatesFiles()) {
    return;
  }

  std::string rotationError;
  {
    std::scoped_lock lock(fileMutex_);
    if (!isFileOpen() || currentEncoder_ == nullptr) {
      return;
    }
    if (++writesSinceLastSizeCheck_ < FILE_SIZE_CHECK_WRITE_INTERVAL) {
      return;
    }
    writesSinceLastSizeCheck_ = 0;

    if (currentEncoder_->getFileSizeBytes() <= fileProperties_->writer.rotateIntervalBytes) {
      return;
    }

    auto closeResult = closeCurrentFile();
    if (closeResult.is_err()) {
      rotationError = closeResult.unwrap_err();
    } else {
      addFinishedFile(closeResult.unwrap());
      auto openResult = openNextFile();
      if (openResult.is_err()) {
        rotationError = openResult.unwrap_err();
      } else {
        sessionFilePaths_.push_back(openResult.unwrap());
      }
    }

    if (!rotationError.empty()) {
      // Nothing more can be encoded, so stop the audio thread from queueing into a dead writer.
      isFileOpen_.store(false, std::memory_order_release);
    }
  }

  // Reaches back into the outside world, so it does not run under the lock.
  if (!rotationError.empty()) {
    invokeOnErrorCallback("Failed to start the next recording segment: " + rotationError);
  }
}

Result<NoneType, std::string> AudioFileWriter::write(const AudioBuffer &frames, size_t numFrames) {
  if (isFileOpen()) {
    worker_.push(frames, numFrames);
  }
  return Ok(None);
}

void AudioFileWriter::runWriterTask(PendingFrames pending) {
  if (pending.buffer == nullptr) {
    return;
  }
  const auto numFrames = static_cast<int>(pending.numFrames);

  std::string encodeError;
  bool encoded = false;
  {
    std::scoped_lock lock(fileMutex_);
    if (isFileOpen() && currentEncoder_ != nullptr) {
      auto result = currentEncoder_->encode(*pending.buffer, numFrames);
      if (result.is_ok()) {
        encoded = true;
      } else {
        encodeError = "Failed to write audio data to file: " + currentEncoder_->getFilePath() +
            " - " + result.unwrap_err();
      }
    }
  }

  // The error callback and a rotation below can take a while; returning the buffer first
  // keeps the audio thread from running out of pool slots meanwhile.
  pending.buffer.reset();

  if (!encodeError.empty()) {
    invokeOnErrorCallback(encodeError);
    return;
  }
  if (encoded) {
    rotateIfFileOutgrowsCap();
  }
}

std::string AudioFileWriter::getFilePath() const {
  std::scoped_lock lock(fileMutex_);
  return currentEncoder_ != nullptr ? currentEncoder_->getFilePath() : std::string();
}

std::vector<std::string> AudioFileWriter::getSessionFilePaths() const {
  std::scoped_lock lock(fileMutex_);
  return sessionFilePaths_;
}

double AudioFileWriter::getCurrentDuration() const {
  std::scoped_lock lock(fileMutex_);
  const double currentFileDurationSec =
      currentEncoder_ != nullptr ? currentEncoder_->getEncodedDurationSeconds() : 0.0;
  return finishedFilesDurationSec_ + currentFileDurationSec;
}

size_t AudioFileWriter::getFileSizeBytes() const {
  std::scoped_lock lock(fileMutex_);
  return currentEncoder_ != nullptr ? currentEncoder_->getFileSizeBytes() : 0;
}

void AudioFileWriter::invokeOnErrorCallback(const std::string &message) {
  if (errorEvent_ != nullptr) {
    errorEvent_->dispatch(StringPayload{.name = "message", .reason = message});
  }
}

bool AudioFileWriter::isFileOpen() const {
  return isFileOpen_.load(std::memory_order_acquire);
}

bool AudioFileWriter::rotatesFiles() const {
  return fileProperties_->writer.rotateIntervalBytes > 0;
}

} // namespace audioapi
