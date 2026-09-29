#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/RecordingFile.h>
#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/FileSystem.hpp>

#ifdef ANDROID
#include <android/log.h>
#endif

#include <array>
#include <cstdio>
#include <cstring>
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
      .createEncoder = &createOsEncoder,
      .reprepareEncoderInput = &reprepareOsEncoderInput,
  };
  return backend;
}

AudioFileWriter::AudioFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    const PlatformFileBackend &backend)
    : fileProperties_(fileProperties), backend_(backend), errorEvent_(audioEventHandlerRegistry) {}

AudioFileWriter::~AudioFileWriter() {
  isFileOpen_.store(false, std::memory_order_release);
  cleanupPreallocatedInputPool();
  currentFile_.reset();
}

OpenFileResult AudioFileWriter::openFile(const StreamFormat &streamFormat) {
  if (isFileOpen()) {
    return OpenFileResult::Err("file already open");
  }

  {
    std::scoped_lock lock(fileMutex_);
    sessionStem_ = recording_file_name::sessionStem(fileProperties_);
    sessionFilePaths_.clear();
    openedFileCount_ = 0;
    finishedFilesSizeMB_ = 0.0;
    finishedFilesDurationSec_ = 0.0;
  }

  return startNextFile(streamFormat);
}

CloseFileResult AudioFileWriter::closeFile() {
  if (!isFileOpen()) {
    return CloseFileResult::Err("file is not open: " + getFilePath());
  }

  auto finished = finishCurrentFile();
  if (finished.is_err()) {
    return CloseFileResult::Err(finished.unwrap_err());
  }

  std::scoped_lock lock(fileMutex_);
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

OpenFileResult AudioFileWriter::reprepareStreamFormat(const StreamFormat &streamFormat) {
  if (!isFileOpen()) {
    return OpenFileResult::Err("file is not open");
  }
  if (streamFormat.sampleRate <= 0 || streamFormat.channelCount <= 0 ||
      streamFormat.maxFramesPerBuffer == 0) {
    return OpenFileResult::Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  cleanupPreallocatedInputPool();
  isFileOpen_.store(false, std::memory_order_release);

  OpenFileResult reprepareResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    streamFormat_ = streamFormat;
    reprepareResult = retargetCurrentFile();
  }
  if (reprepareResult.is_err()) {
    // Nothing more can be encoded in the new format, so the file is finished as far as it got.
    finishCurrentFile();
    return reprepareResult;
  }

  if (!initializePreallocatedInputPool()) {
    finishCurrentFile();
    return OpenFileResult::Err("Failed to preallocate file writer buffers");
  }

  isFileOpen_.store(true, std::memory_order_release);
  return reprepareResult;
}

OpenFileResult AudioFileWriter::startNextFile(const StreamFormat &streamFormat) {
  if (streamFormat.sampleRate <= 0 || streamFormat.channelCount <= 0 ||
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

  if (!initializePreallocatedInputPool()) {
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
  cleanupPreallocatedInputPool();
  isFileOpen_.store(false, std::memory_order_release);

  std::scoped_lock lock(fileMutex_);
  auto closeResult = closeCurrentFile();

  if (closeResult.is_ok()) {
    foldFinishedFile(closeResult.unwrap());
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

  auto encoder = backend_.createEncoder(
      EncoderSettings{.stream = fileProperties_->stream, .encoding = fileProperties_->encoding});
  if (encoder == nullptr) {
    return OpenFileResult::Err("Audio file recording requires iOS or Android.");
  }
  auto fileResult = RecordingFile::open(std::move(encoder), streamFormat_, outputSpec, filePath);
  if (fileResult.is_err()) {
    return OpenFileResult::Err(fileResult.unwrap_err());
  }

  currentFile_ = std::move(fileResult).unwrap();
  ++openedFileCount_;
  writesSinceLastSizeCheck_ = 0;
  return OpenFileResult::Ok(currentFile_->path());
}

OpenFileResult AudioFileWriter::retargetCurrentFile() {
  if (currentFile_ == nullptr) {
    return OpenFileResult::Err("file is not open");
  }

  if (!backend_.reprepareEncoderInput) {
    return OpenFileResult::Err("The platform cannot change the input format of an open file");
  }

  auto result = currentFile_->changeInputFormat(streamFormat_, backend_.reprepareEncoderInput);
  if (result.is_err()) {
    return OpenFileResult::Err(
        "Failed to switch the recording to the new input format: " + result.unwrap_err());
  }
  return OpenFileResult::Ok(currentFile_->path());
}

CloseEncoderResult AudioFileWriter::closeCurrentFile() {
  if (currentFile_ == nullptr) {
    return CloseEncoderResult::Err("file is not open");
  }

  auto closeResult = currentFile_->close();
  currentFile_.reset();
  return closeResult;
}

void AudioFileWriter::foldFinishedFile(const std::tuple<double, double> &finished) {
  finishedFilesSizeMB_ += std::get<0>(finished);
  finishedFilesDurationSec_ += std::get<1>(finished);
}

void AudioFileWriter::rollbackFailedOpen() {
  cleanupPreallocatedInputPool();

  std::scoped_lock lock(fileMutex_);
  if (currentFile_ != nullptr) {
    currentFile_->discard();
    currentFile_.reset();
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
    if (!isFileOpen() || currentFile_ == nullptr) {
      return;
    }
    if (++writesSinceLastSizeCheck_ < FILE_SIZE_CHECK_WRITE_INTERVAL) {
      return;
    }
    writesSinceLastSizeCheck_ = 0;

    if (currentFile_->sizeBytes() <= fileProperties_->writer.rotateIntervalBytes) {
      return;
    }

    auto closeResult = closeCurrentFile();
    if (closeResult.is_err()) {
      rotationError = closeResult.unwrap_err();
    } else {
      foldFinishedFile(closeResult.unwrap());
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

void AudioFileWriter::createOffloader() {
  auto offloaderLambda = [this](PendingFileWrite pending) {
    runWriterTask(std::move(pending));
  };
  offloader_ = std::make_unique<Offloader>(CHANNEL_CAPACITY, offloaderLambda);
}

bool AudioFileWriter::initializePreallocatedInputPool() {
  cleanupPreallocatedInputPool();

  if (streamFormat_.maxFramesPerBuffer == 0 || streamFormat_.channelCount <= 0 ||
      streamFormat_.channelCount > MAX_CHANNEL_COUNT) {
    return false;
  }

  if (!inputBufferPool_.allocate(
          streamFormat_.maxFramesPerBuffer, streamFormat_.channelCount, streamFormat_.sampleRate)) {
    return false;
  }

  // Last, so the worker never sees a half-built pool.
  createOffloader();
  return true;
}

void AudioFileWriter::cleanupPreallocatedInputPool() {
  // Stop the worker before freeing the pool it reads from.
  offloader_.reset();
  inputBufferPool_.clear();
}

void AudioFileWriter::writeAudioData(const float *const *channels, int numFrames) {
  if (!isFileOpen() || channels == nullptr || numFrames <= 0 || offloader_ == nullptr) {
    return;
  }

  AudioBufferLease slot = inputBufferPool_.tryAcquire();
  if (slot == nullptr) {
    return;
  }

  const auto frames = static_cast<size_t>(numFrames);
  if (frames > slot->getSize()) {
    return;
  }

  for (int channel = 0; channel < streamFormat_.channelCount; ++channel) {
    std::memcpy(slot->getChannel(channel)->begin(), channels[channel], frames * sizeof(float));
  }
  // Never blocks: the channel has room for every buffer the pool can hand out.
  offloader_->getSender()->send(
      PendingFileWrite{.buffer = std::move(slot), .numFrames = numFrames});
}

void AudioFileWriter::runWriterTask(PendingFileWrite pending) {
  if (pending.buffer == nullptr) {
    return;
  }
  const int numFrames = pending.numFrames;

  std::array<const float *, MAX_CHANNEL_COUNT> channels{};
  for (int channel = 0; channel < streamFormat_.channelCount; ++channel) {
    channels[channel] = pending.buffer->getChannel(channel)->begin();
  }

  std::string encodeError;
  bool encoded = false;
  {
    std::scoped_lock lock(fileMutex_);
    if (isFileOpen() && currentFile_ != nullptr) {
      auto result = currentFile_->encode(channels.data(), numFrames);
      if (result.is_ok()) {
        encoded = true;
      } else {
        encodeError = "Failed to write audio data to file: " + currentFile_->path() + " - " +
            result.unwrap_err();
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
  return currentFile_ != nullptr ? currentFile_->path() : std::string();
}

std::vector<std::string> AudioFileWriter::getSessionFilePaths() const {
  std::scoped_lock lock(fileMutex_);
  return sessionFilePaths_;
}

double AudioFileWriter::getCurrentDuration() const {
  std::scoped_lock lock(fileMutex_);
  const double currentFileDurationSec = currentFile_ != nullptr ? currentFile_->durationSec() : 0.0;
  return finishedFilesDurationSec_ + currentFileDurationSec;
}

size_t AudioFileWriter::getFileSizeBytes() const {
  std::scoped_lock lock(fileMutex_);
  return currentFile_ != nullptr ? currentFile_->sizeBytes() : 0;
}

void AudioFileWriter::setOnErrorCallback(uint64_t callbackId) {
  errorEvent_.assignCallbackId(callbackId);
}

void AudioFileWriter::clearOnErrorCallback() {
  errorEvent_.assignCallbackId(0);
}

void AudioFileWriter::invokeOnErrorCallback(const std::string &message) {
  errorEvent_.dispatch(StringPayload{.name = "message", .reason = message});
}

bool AudioFileWriter::isFileOpen() const {
  return isFileOpen_.load(std::memory_order_acquire);
}

bool AudioFileWriter::rotatesFiles() const {
  return fileProperties_->writer.rotateIntervalBytes > 0;
}

} // namespace audioapi
