#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/RecordingFileName.h>
#include <audioapi/encoding/EncoderCapabilities.h>
#include <audioapi/encoding/OSEncoding.h>
#include <audioapi/encoding/OSFilePath.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/AudioFileProperties.h>

#ifdef ANDROID
#include <android/log.h>
#endif

#include <sys/stat.h>
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

PlatformFileBackend createOsFileBackend() {
  return PlatformFileBackend{
      .resolveOutputSpec = &EncoderCapabilities::resolveOutputSpec,
      .resolvePath = &resolveOsFilePath,
      .createEncoder = &createOsEncoder,
      .reprepareEncoderInput = &reprepareOsEncoderInput,
  };
}

AudioFileWriter::AudioFileWriter(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties,
    PlatformFileBackend backend)
    : fileProperties_(fileProperties),
      backend_(std::move(backend)),
      errorEvent_(audioEventHandlerRegistry) {}

AudioFileWriter::~AudioFileWriter() {
  isFileOpen_.store(false, std::memory_order_release);
  cleanupPreallocatedInputPool();
  encoder_.reset();
}

OpenFileResult AudioFileWriter::openFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (isFileOpen()) {
    return OpenFileResult::Err("file already open");
  }

  {
    std::scoped_lock lock(fileMutex_);
    sessionStem_ = recordingfilename::sessionStem(fileProperties_);
    sessionFilePaths_.clear();
    openedFileCount_ = 0;
    finishedFilesSizeMB_ = 0.0;
    finishedFilesDurationSec_ = 0.0;
  }

  return startNextFile(streamSampleRate, streamChannelCount, maxFramesPerBuffer);
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

OpenFileResult AudioFileWriter::reprepareStreamFormat(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (!isFileOpen()) {
    return OpenFileResult::Err("file is not open");
  }
  if (streamSampleRate <= 0 || streamChannelCount <= 0 || maxFramesPerBuffer <= 0) {
    return OpenFileResult::Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  cleanupPreallocatedInputPool();
  isFileOpen_.store(false, std::memory_order_release);

  OpenFileResult reprepareResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    if (streamSampleRate_ > 0) {
      currentFileEarlierFormatsDurationSec_ +=
          static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / streamSampleRate_;
    }
    framesWritten_.store(0, std::memory_order_release);
    streamSampleRate_ = streamSampleRate;
    streamChannelCount_ = streamChannelCount;
    maxFramesPerBuffer_ = maxFramesPerBuffer;
    reprepareResult = reprepareEncoderInput();
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

OpenFileResult AudioFileWriter::startNextFile(
    float streamSampleRate,
    int32_t streamChannelCount,
    int32_t maxFramesPerBuffer) {
  if (fileProperties_->stream.sampleRate <= 0 || fileProperties_->stream.channelCount <= 0) {
    return OpenFileResult::Err(
        "Invalid file properties: sampleRate and channelCount must be greater than 0");
  }
  if (streamSampleRate <= 0 || streamChannelCount <= 0 || maxFramesPerBuffer <= 0) {
    return OpenFileResult::Err(
        "Invalid input format: sampleRate, channelCount and buffer size must be greater than 0");
  }

  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxFramesPerBuffer_ = maxFramesPerBuffer;
  framesWritten_.store(0, std::memory_order_release);

  OpenFileResult openResult = OpenFileResult::Err("");
  {
    std::scoped_lock lock(fileMutex_);
    openResult = openEncoderForNextFile();
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
  auto closeResult = retireEncoder();
  filePath_ = "";

  if (closeResult.is_ok()) {
    foldFinishedFile(closeResult.unwrap());
  }
  return closeResult;
}

std::string AudioFileWriter::fileStem(size_t fileNumber) const {
  if (rotatesFiles()) {
    return recordingfilename::segmentStem(sessionStem_, fileNumber);
  }
  return sessionStem_;
}

static bool fileExists(const std::string &path) {
  struct stat existing{};
  return ::stat(path.c_str(), &existing) == 0;
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
    if (fileExists(pathResult.unwrap())) {
      warnAboutOverwrite(pathResult.unwrap());
    }
    return pathResult;
  }

  for (size_t suffix = 1; fileExists(pathResult.unwrap()); ++suffix) {
    std::string suffixedName = stem;
    suffixedName += "_" + std::to_string(suffix) + "." + extension;
    pathResult = backend_.resolvePath(fileProperties_, suffixedName);
    if (pathResult.is_err()) {
      return pathResult;
    }
  }
  return pathResult;
}

OpenFileResult AudioFileWriter::openEncoderForNextFile() {
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

  const StreamFormat inputFormat{
      .sampleRate = streamSampleRate_,
      .channelCount = streamChannelCount_,
  };

  auto encoder = backend_.createEncoder(fileProperties_);
  if (encoder == nullptr) {
    return OpenFileResult::Err("Audio file recording requires iOS or Android.");
  }
  auto openResult =
      encoder->open(inputFormat, outputSpec, static_cast<size_t>(maxFramesPerBuffer_), filePath);
  if (openResult.is_err()) {
    return OpenFileResult::Err(openResult.unwrap_err());
  }

  encoder_ = std::move(encoder);
  filePath_ = filePath;
  ++openedFileCount_;
  writesSinceLastSizeCheck_ = 0;
  currentFileEarlierFormatsDurationSec_ = 0.0;
  framesWritten_.store(0, std::memory_order_release);

  return OpenFileResult::Ok(filePath_);
}

OpenFileResult AudioFileWriter::reprepareEncoderInput() {
  if (encoder_ == nullptr) {
    return OpenFileResult::Err("file is not open: " + filePath_);
  }

  if (!backend_.reprepareEncoderInput) {
    return OpenFileResult::Err("The platform cannot change the input format of an open file");
  }

  const StreamFormat inputFormat{
      .sampleRate = streamSampleRate_,
      .channelCount = streamChannelCount_,
  };
  auto result = backend_.reprepareEncoderInput(
      *encoder_, inputFormat, static_cast<size_t>(maxFramesPerBuffer_));
  if (result.is_err()) {
    return OpenFileResult::Err(
        "Failed to switch the recording to the new input format: " + result.unwrap_err());
  }
  return OpenFileResult::Ok(filePath_);
}

CloseEncoderResult AudioFileWriter::retireEncoder() {
  if (encoder_ == nullptr) {
    return CloseEncoderResult::Err("file is not open: " + filePath_);
  }

  auto closeResult = encoder_->close();
  encoder_.reset();
  currentFileEarlierFormatsDurationSec_ = 0.0;
  framesWritten_.store(0, std::memory_order_release);
  return closeResult;
}

void AudioFileWriter::foldFinishedFile(const std::tuple<double, double> &finished) {
  finishedFilesSizeMB_ += std::get<0>(finished);
  finishedFilesDurationSec_ += std::get<1>(finished);
}

void AudioFileWriter::rollbackFailedOpen() {
  cleanupPreallocatedInputPool();

  std::scoped_lock lock(fileMutex_);
  // Whatever the encoder reports about a file that is about to be deleted is of no use.
  retireEncoder();
  if (!filePath_.empty()) {
    std::remove(filePath_.c_str());
    filePath_ = "";
  }
  isFileOpen_.store(false, std::memory_order_release);
}

void AudioFileWriter::rotateOnceFileOutgrowsCap() {
  if (!rotatesFiles()) {
    return;
  }

  std::string rotationError;
  {
    std::scoped_lock lock(fileMutex_);
    if (!isFileOpen() || encoder_ == nullptr) {
      return;
    }
    if (++writesSinceLastSizeCheck_ < FILE_SIZE_CHECK_WRITE_INTERVAL) {
      return;
    }
    writesSinceLastSizeCheck_ = 0;

    if (encoder_->getFileSizeBytes() <= fileProperties_->writer.rotateIntervalBytes) {
      return;
    }

    auto closeResult = retireEncoder();
    if (closeResult.is_err()) {
      rotationError = closeResult.unwrap_err();
    } else {
      foldFinishedFile(closeResult.unwrap());
      auto openResult = openEncoderForNextFile();
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
  offloader_ = std::make_unique<Offloader>(FILE_WRITER_CHANNEL_CAPACITY, offloaderLambda);
}

bool AudioFileWriter::initializePreallocatedInputPool() {
  cleanupPreallocatedInputPool();

  if (maxFramesPerBuffer_ <= 0 || streamChannelCount_ <= 0 ||
      streamChannelCount_ > MAX_CHANNEL_COUNT) {
    return false;
  }

  if (!inputBufferPool_.allocate(
          static_cast<size_t>(maxFramesPerBuffer_), streamChannelCount_, streamSampleRate_)) {
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

  for (int channel = 0; channel < streamChannelCount_; ++channel) {
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
  for (int channel = 0; channel < streamChannelCount_; ++channel) {
    channels[channel] = pending.buffer->getChannel(channel)->begin();
  }

  std::string encodeError;
  bool encoded = false;
  {
    std::scoped_lock lock(fileMutex_);
    if (isFileOpen() && encoder_ != nullptr) {
      auto result = encoder_->encode(channels.data(), numFrames);
      if (result.is_ok()) {
        framesWritten_.fetch_add(numFrames, std::memory_order_acq_rel);
        encoded = true;
      } else {
        encodeError =
            "Failed to write audio data to file: " + filePath_ + " - " + result.unwrap_err();
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
    rotateOnceFileOutgrowsCap();
  }
}

std::string AudioFileWriter::getFilePath() const {
  std::scoped_lock lock(fileMutex_);
  return filePath_;
}

std::vector<std::string> AudioFileWriter::getSessionFilePaths() const {
  std::scoped_lock lock(fileMutex_);
  return sessionFilePaths_;
}

double AudioFileWriter::getCurrentDuration() const {
  std::scoped_lock lock(fileMutex_);
  const double sampleRate =
      streamSampleRate_ > 0 ? streamSampleRate_ : fileProperties_->stream.sampleRate;
  if (sampleRate <= 0) {
    return finishedFilesDurationSec_ + currentFileEarlierFormatsDurationSec_;
  }
  const double currentFormatDurationSec =
      static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / sampleRate;
  return finishedFilesDurationSec_ + currentFileEarlierFormatsDurationSec_ +
      currentFormatDurationSec;
}

size_t AudioFileWriter::getFileSizeBytes() const {
  std::scoped_lock lock(fileMutex_);
  if (encoder_ != nullptr) {
    return encoder_->getFileSizeBytes();
  }
  struct stat st{};
  if (!filePath_.empty() && stat(filePath_.c_str(), &st) == 0) {
    return static_cast<size_t>(st.st_size);
  }
  return 0;
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
