#import <AVFoundation/AVFoundation.h>
#import <AudioEngine.h>
#import <AudioSessionManager.h>
#import <Foundation/Foundation.h>

#include <unordered_map>
#include <vector>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/IOSRecorderCallback.h>
#include <audioapi/ios/core/utils/IOSRotatingFileWriter.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <audioapi/utils/Result.hpp>

namespace audioapi {

static inline NSNumber *recorderFormatNumber(id format, NSString *key)
{
  if (format == nil) {
    return nil;
  }

  if ([format isKindOfClass:[AVAudioFormat class]]) {
    AVAudioFormat *audioFormat = (AVAudioFormat *)format;

    if ([key isEqualToString:@"sampleRate"]) {
      return @(audioFormat.sampleRate);
    }

    if ([key isEqualToString:@"channelCount"]) {
      return @(audioFormat.channelCount);
    }

    if ([key isEqualToString:@"interleaved"]) {
      return @(audioFormat.interleaved);
    }
  }

  @try {
    id value = [format valueForKey:key];
    return [value isKindOfClass:[NSNumber class]] ? value : nil;
  } @catch (__unused NSException *exception) {
    return nil;
  }
}

static inline double recorderFormatSampleRate(id format)
{
  return [recorderFormatNumber(format, @"sampleRate") doubleValue];
}

static inline AVAudioChannelCount recorderFormatChannelCount(id format)
{
  return (AVAudioChannelCount)[recorderFormatNumber(format, @"channelCount") unsignedIntegerValue];
}

static inline bool recorderFormatInterleaved(id format)
{
  return [recorderFormatNumber(format, @"interleaved") boolValue];
}

static inline bool hasUsableRecorderFormat(id format)
{
  return format != nil && recorderFormatSampleRate(format) > 0 &&
      recorderFormatChannelCount(format) > 0;
}

static std::string describeRecorderFormat(id format)
{
  return "engineFormat={sampleRate=" + std::to_string(recorderFormatSampleRate(format)) +
      ", channelCount=" + std::to_string(recorderFormatChannelCount(format)) +
      ", interleaved=" + std::string(recorderFormatInterleaved(format) ? "true" : "false") + "}";
}

static void cleanupStartedRecorder(
    NativeAudioRecorder *nativeRecorder,
    const std::shared_ptr<AudioFileWriter> &fileWriter,
    bool fileWasOpened)
{
  [nativeRecorder setInputArmed:false];
  [nativeRecorder stop];

  if (fileWasOpened && fileWriter != nullptr) {
    fileWriter->closeFile();
  }
}

/// @brief Constructs an IOSAudioRecorder instance.
/// This constructor initializes the receiver block and native side recorder wrapper (AVAudioSinkNode).
/// All other necessary fields (like buffers) are initialized in start() method.
/// This "method" should be called from the JS thread only.
/// @param audioEventHandlerRegistry Shared pointer to the AudioEventHandlerRegistry for event handling.
IOSAudioRecorder::IOSAudioRecorder(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
    : AudioRecorder(audioEventHandlerRegistry)
{
  AudioReceiverBlock receiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames) {
    if (usesFileOutput()) {
      if (auto lock = Locker::tryLock(fileWriterMutex_)) {
        fileWriter_->writeAudioData(inputBuffer, numFrames);
      }
    }

    if (usesCallback()) {
      if (auto lock = Locker::tryLock(callbackMutex_)) {
        std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
            ->receiveAudioData(inputBuffer, numFrames);
      }
    }

    if (isConnected()) {
      if (auto lock = Locker::tryLock(adapterNodeMutex_)) {
        for (size_t channel = 0; channel < adapterNode_->getChannelCount(); ++channel) {
          auto data = (float *)inputBuffer->mBuffers[channel].mData;

          adapterNode_->buff_[channel]->write(data, numFrames);
        }
      }
    }
  };

  nativeRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:receiverBlock];
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  stop();

  {
    std::scoped_lock lock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);
    callbackOutputConfigured_.store(false, std::memory_order_release);
    callbackOutputEnabled_.store(false, std::memory_order_release);
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileOutputEnabled_.store(false, std::memory_order_release);
    connectedConfigured_.store(false, std::memory_order_release);
    isConnected_.store(false, std::memory_order_release);
    dataCallback_ = nullptr;
    fileWriter_ = nullptr;
    adapterNode_ = nullptr;
  }

  [nativeRecorder_ cleanup];
}

/// @brief Starts the audio recording process and prepares necessary resources.
/// This method should be called from the JS thread only.
/// @returns Result containing the file path if recording started successfully, or an error message.
Result<NoneType, std::string> IOSAudioRecorder::start(const std::string &fileNameOverride)
{
  if (!isIdle()) {
    return Result<NoneType, std::string>::Err("Recorder is already recording");
  }

  std::scoped_lock startLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);
  AudioSessionManager *audioSessionManager = [AudioSessionManager sharedInstance];

  if ([[audioSessionManager checkRecordingPermissions] isEqual:@"Denied"]) {
    return Result<NoneType, std::string>::Err("Microphone permissions are not granted");
  }

  NSError *nativeStartError = nil;
  BOOL didStartNativeRecorder = NO;

  @try {
    didStartNativeRecorder = [nativeRecorder_ start:&nativeStartError];
  } @catch (NSException *exception) {
    cleanupStartedRecorder(nativeRecorder_, fileWriter_, false);

    std::string message = "Failed to start native recorder";
    message += ": exception={name=";
    message += [[exception.name description] UTF8String];
    message += ", reason=";
    message += [[(exception.reason ?: @"<none>") description] UTF8String];
    message += "}; ";
    message += [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];

#if TARGET_OS_SIMULATOR
    message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

    return Result<NoneType, std::string>::Err(message);
  }

  if (!didStartNativeRecorder) {
    std::string message = "Failed to start native recorder";

    if (nativeStartError != nil) {
      message += ": ";
      message += [[nativeStartError debugDescription] UTF8String];
    }

    message += "; ";
    message += [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];

#if TARGET_OS_SIMULATOR
    message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

    return Result<NoneType, std::string>::Err(message);
  }

  AVAudioFormat *inputFormat = [nativeRecorder_ getResolvedInputFormat];

  if (!hasUsableRecorderFormat(inputFormat)) {
    std::string message = "Audio input format is unavailable. " +
        describeRecorderFormat(inputFormat) + "; " +
        [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];
#if TARGET_OS_SIMULATOR
    message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

    cleanupStartedRecorder(nativeRecorder_, fileWriter_, false);
    return Result<NoneType, std::string>::Err(message);
  }

  // Estimate the maximum input buffer lengths that can be expected from the sink node
  size_t maxInputBufferLength = [nativeRecorder_ getResolvedBufferSize];
  bool fileWasOpened = false;

  if (wantsFileOutput()) {
    recordingSegmentPaths_.clear();
    auto writerResult = setupFileWriter(fileProperties_, fileNameOverride);
    if (writerResult.is_err()) {
      cleanupStartedRecorder(nativeRecorder_, fileWriter_, false);
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter_ = nullptr;
      return Result<NoneType, std::string>::Err(writerResult.unwrap_err());
    }
    filePath_ = writerResult.unwrap();
    fileWasOpened = true;
  }

  if (wantsCallback()) {
    if (dataCallback_ == nullptr) {
      cleanupStartedRecorder(nativeRecorder_, fileWriter_, fileWasOpened);
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter_ = nullptr;
      filePath_ = "";
      return Result<NoneType, std::string>::Err(
          "Failed to prepare callback: callback is unavailable");
    }

    dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
    auto callbackResult = std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
                              ->prepare(inputFormat, maxInputBufferLength);

    if (callbackResult.is_err()) {
      cleanupStartedRecorder(nativeRecorder_, fileWriter_, fileWasOpened);
      callbackOutputConfigured_.store(false, std::memory_order_release);
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter_ = nullptr;
      filePath_ = "";
      return Result<NoneType, std::string>::Err(
          "Failed to prepare callback: " + callbackResult.unwrap_err());
    }

    callbackOutputConfigured_.store(true, std::memory_order_release);
  }

  if (wantsConnection() && adapterNode_ != nullptr) {
    adapterNode_->init(
        maxInputBufferLength,
        recorderFormatChannelCount(inputFormat),
        recorderFormatSampleRate(inputFormat));
    connectedConfigured_.store(true, std::memory_order_release);
  }

  [nativeRecorder_ setInputArmed:true];
  state_.store(RecorderState::Recording, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Stops the audio recording process and releases resources.
/// It finalizes any data receiver and closes the stream.
/// This method should be called from the JS thread only.
/// @returns Result containing paths, size, and duration if stopped successfully, or an error message.
Result<std::tuple<std::vector<std::string>, double, double>, std::string> IOSAudioRecorder::stop()
{
  std::shared_ptr<AudioFileWriter> fileWriter;
  std::shared_ptr<AudioRecorderCallback> dataCallback;
  std::shared_ptr<RecorderAdapterNode> adapterNode;
  std::vector<std::string> outputPaths;
  std::string filePath;

  double outputFileSize = 0;
  double outputDuration = 0;
  bool hadFileOutput = false;

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

    if (isIdle()) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Recorder is not in recording state.");
    }

    [nativeRecorder_ setInputArmed:false];
    state_.store(RecorderState::Idle, std::memory_order_release);
    [nativeRecorder_ stop];

    hadFileOutput = usesFileOutput();
    bool hadCallback = usesCallback();
    bool hadConnection = isConnected();
    filePath = filePath_;

    if (hadFileOutput) {
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter = std::move(fileWriter_);
    }

    if (hadCallback) {
      callbackOutputConfigured_.store(false, std::memory_order_release);
      dataCallback = std::move(dataCallback_);
    }

    if (hadConnection) {
      connectedConfigured_.store(false, std::memory_order_release);
      adapterNode = std::move(adapterNode_);
    }

    for (const auto &raw : recordingSegmentPaths_) {
      if (!raw.empty()) {
        outputPaths.push_back(std::string("file://") + raw);
      }
    }
    if (hadFileOutput && outputPaths.empty() && !filePath.empty()) {
      outputPaths.push_back(std::string("file://") + filePath);
    }

    recordingSegmentPaths_.clear();
    filePath_ = "";
  }

  if (fileWriter != nullptr) {
    auto fileResult = fileWriter->closeFile();

    if (fileResult.is_err()) {
      return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Err(
          "Failed to close file: " + fileResult.unwrap_err());
    }

    outputFileSize = std::get<0>(fileResult.unwrap());
    outputDuration = std::get<1>(fileResult.unwrap());
  }

  if (dataCallback != nullptr) {
    dataCallback->cleanup();
  }

  if (adapterNode != nullptr) {
    adapterNode->adapterCleanup();
  }

  return Result<std::tuple<std::vector<std::string>, double, double>, std::string>::Ok(
      std::make_tuple(std::move(outputPaths), outputFileSize, outputDuration));
}

/// @brief Enables file output for the recorder with specified properties.
/// If the recorder is already active, it will open the file for writing immediately.
/// This method should be called from the JS thread only.
/// @param properties Shared pointer to AudioFileProperties defining the output file format.
/// @returns Result containing the output file path if enabled successfully, or an error message.
Result<NoneType, std::string> IOSAudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties)
{
  std::scoped_lock lock(fileWriterMutex_, errorCallbackMutex_);
  fileProperties_ = properties;
  fileOutputEnabled_.store(true, std::memory_order_release);
  fileOutputConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    AVAudioFormat *resolvedInputFormat = [nativeRecorder_ getResolvedInputFormat];
    int resolvedBufferSize = [nativeRecorder_ getResolvedBufferSize];

    if (!hasUsableRecorderFormat(resolvedInputFormat) || resolvedBufferSize <= 0) {
      return Result<NoneType, std::string>::Err(
          "Failed to open file for writing: recorder input format is unavailable");
    }

    auto writerResult = setupFileWriter(properties);
    if (writerResult.is_err()) {
      fileOutputEnabled_.store(false, std::memory_order_release);
      return Result<NoneType, std::string>::Err(writerResult.unwrap_err());
    }
  }

  return Result<NoneType, std::string>::Ok(None);
}

std::shared_ptr<AudioFileWriter> IOSAudioRecorder::createFileWriter(
    const std::shared_ptr<AudioFileProperties> &props)
{
  return std::make_shared<IOSFileWriter>(audioEventHandlerRegistry_, props);
}

Result<std::string, std::string> IOSAudioRecorder::setupFileWriter(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride)
{
  if (properties->rotateIntervalBytes > 0) {
    fileWriter_ = std::make_shared<IOSRotatingFileWriter>(
        audioEventHandlerRegistry_,
        properties,
        properties->rotateIntervalBytes,
        [this](const std::shared_ptr<AudioFileProperties> &p) { return createFileWriter(p); },
        [this](const std::string &path) {
          if (!path.empty()) {
            recordingSegmentPaths_.push_back(path);
          }
        });
  } else {
    fileWriter_ = createFileWriter(properties);
  }

  fileWriter_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));

  auto backend = std::static_pointer_cast<IOSFileWriter>(fileWriter_);
  auto fileResult = backend->openFile(
      [nativeRecorder_ getResolvedInputFormat],
      [nativeRecorder_ getResolvedBufferSize],
      fileNameOverride);

  if (!fileResult.is_ok()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    fileWriter_ = nullptr;
    return Result<std::string, std::string>::Err(
        "Failed to open file for writing: " + fileResult.unwrap_err());
  }

  filePath_ = fileResult.unwrap();

  if (properties->rotateIntervalBytes == 0) {
    recordingSegmentPaths_.push_back(filePath_);
  }
  fileOutputConfigured_.store(true, std::memory_order_release);
  return Result<std::string, std::string>::Ok(filePath_);
}

void IOSAudioRecorder::disableFileOutput()
{
  std::scoped_lock lock(fileWriterMutex_);
  fileOutputConfigured_.store(false, std::memory_order_release);
  fileOutputEnabled_.store(false, std::memory_order_release);

  if (fileWriter_ != nullptr) {
    fileWriter_->closeFile();
  }
}

/// @brief Connects a RecorderAdapterNode to the recorder for audio data routing.
/// If the recorder is already active, it will initialize the adapter node immediately.
/// This method should be called from the JS thread only.
/// @param node Shared pointer to the RecorderAdapterNode to connect.
void IOSAudioRecorder::connect(const std::shared_ptr<RecorderAdapterNode> &node)
{
  std::scoped_lock lock(adapterNodeMutex_);
  adapterNode_ = node;
  isConnected_.store(true, std::memory_order_release);
  connectedConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    AVAudioFormat *resolvedInputFormat = [nativeRecorder_ getResolvedInputFormat];

    if (!hasUsableRecorderFormat(resolvedInputFormat)) {
      return;
    }

    adapterNode_->init(
        [nativeRecorder_ getResolvedBufferSize],
        resolvedInputFormat.channelCount,
        resolvedInputFormat.sampleRate);
    connectedConfigured_.store(true, std::memory_order_release);
  }
}

/// @brief Disconnects the currently connected RecorderAdapterNode from the recorder.
/// If the recorder is currently active, it will stop routing audio data immediately.
/// This method should be called from the JS thread only.
void IOSAudioRecorder::disconnect()
{
  std::shared_ptr<RecorderAdapterNode> adapterNode;
  bool hadConnection = false;
  {
    std::scoped_lock lock(adapterNodeMutex_);
    hadConnection = isConnected();
    connectedConfigured_.store(false, std::memory_order_release);
    isConnected_.store(false, std::memory_order_release);
    adapterNode = std::move(adapterNode_);
  }

  if (hadConnection && adapterNode != nullptr) {
    adapterNode->adapterCleanup();
  }
}

void IOSAudioRecorder::pause()
{
  if (!isRecording()) {
    return;
  }

  [nativeRecorder_ pause];
  state_.store(RecorderState::Paused, std::memory_order_release);
}

void IOSAudioRecorder::resume()
{
  if (!isPaused()) {
    return;
  }

  [nativeRecorder_ resume];
  state_.store(RecorderState::Recording, std::memory_order_release);
}

/// @brief Checks if the recorder is currently recording.
/// Besides recorder internal state, it also check if the audio engine is running.
/// this helps with restarts after interruptions or other audio session changes.
/// This method can be called from any thread.
/// @returns True if recording, false otherwise.
bool IOSAudioRecorder::isRecording() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  return state_.load(std::memory_order_acquire) == RecorderState::Recording &&
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
}

/// @brief Checks if the recorder is currently paused.
/// Besides recorder internal state, it also check if the audio engine is running.
/// this helps with restarts after interruptions or other audio session changes.
/// This method can be called from any thread.
/// @returns True if paused, false otherwise.
bool IOSAudioRecorder::isPaused() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  auto currentState = state_.load(std::memory_order_acquire);

  if (currentState == RecorderState::Idle) {
    return false;
  }

  return currentState == RecorderState::Paused ||
      [audioEngine getState] != AudioEngineState::AudioEngineStateRunning;
}

/// @brief Checks if the recorder is currently idle (not recording or paused).
/// This method can be called from any thread.
/// @returns True if idle, false otherwise.
bool IOSAudioRecorder::isIdle() const
{
  return state_.load(std::memory_order_acquire) == RecorderState::Idle;
}

/// @brief Sets the callback to be invoked when audio data is ready.
/// If the recorder is already active, it will prepare the callback for receiving audio data immediately.
/// This method should be called from the JS thread only.
/// @param sampleRate Desired sample rate for the callback audio data.
/// @param bufferLength Desired buffer length in frames for the callback audio data.
/// @param channelCount Number of channels for the callback audio data.
/// @param callbackId Identifier for the JS callback to be invoked.
/// @returns Success status or Error status with message.
Result<NoneType, std::string> IOSAudioRecorder::setOnAudioReadyCallback(
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId)
{
  std::scoped_lock lock(callbackMutex_, errorCallbackMutex_);

  dataCallback_ = std::make_shared<IOSRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  dataCallback_->setOnErrorCallback(errorCallbackId_.load(std::memory_order_acquire));
  callbackOutputEnabled_.store(true, std::memory_order_release);
  callbackOutputConfigured_.store(false, std::memory_order_release);

  if (!isIdle()) {
    AVAudioFormat *resolvedInputFormat = [nativeRecorder_ getResolvedInputFormat];

    if (!hasUsableRecorderFormat(resolvedInputFormat)) {
      return Result<NoneType, std::string>::Err("Recorder input format is unavailable");
    }

    auto result = std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
                      ->prepare(resolvedInputFormat, [nativeRecorder_ getResolvedBufferSize]);

    if (result.is_err()) {
      callbackOutputEnabled_.store(false, std::memory_order_release);
      callbackOutputConfigured_.store(false, std::memory_order_release);
      dataCallback_ = nullptr;
      return Result<NoneType, std::string>::Err(result.unwrap_err());
    }

    callbackOutputConfigured_.store(true, std::memory_order_release);
  }
  return Result<NoneType, std::string>::Ok(None);
}

/// @brief Clears the audio data callback.
/// If the recorder is currently active, it will stop invoking the callback immediately.
/// This method should be called from the JS thread only.
void IOSAudioRecorder::clearOnAudioReadyCallback()
{
  std::scoped_lock lock(callbackMutex_);
  callbackOutputConfigured_.store(false, std::memory_order_release);
  callbackOutputEnabled_.store(false, std::memory_order_release);
  dataCallback_ = nullptr;
}

} // namespace audioapi
