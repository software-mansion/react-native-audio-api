#import <AVFoundation/AVFoundation.h>
#import <AudioEngine.h>
#import <AudioSessionManager.h>
#import <Foundation/Foundation.h>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>
#include <audioapi/ios/core/utils/IOSRecorderCallback.h>
#include <audioapi/ios/core/utils/IOSRotatingFileWriter.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/Result.hpp>

namespace audioapi {

static double recorderFormatSampleRate(AVAudioFormat *format)
{
  return format.sampleRate;
}

static AVAudioChannelCount recorderFormatChannelCount(AVAudioFormat *format)
{
  return format.channelCount;
}

static bool hasUsableRecorderFormat(AVAudioFormat *format)
{
  return format != nil && format.sampleRate > 0 && format.channelCount > 0;
}

static std::string describeRecorderFormat(AVAudioFormat *format)
{
  return "engineFormat={sampleRate=" + std::to_string(format.sampleRate) +
      ", channelCount=" + std::to_string(format.channelCount) +
      ", interleaved=" + (format.interleaved ? "true" : "false") + "}";
}

/// @brief Appends the shared input diagnostics (and, on the simulator, the host-mic hint)
/// to a start() failure message so every early-return path reports the same context.
static std::string withStartDiagnostics(
    std::string message,
    AudioSessionManager *audioSessionManager)
{
  message += "; ";
  message += [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];

#if TARGET_OS_SIMULATOR
  message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

  return message;
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
    runSideEffects(inputBuffer, numFrames);
  };

  nativeRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:receiverBlock];

  nativeRecorder_.onInputConfigurationChange = ^{ this->handleInputConfigurationChange(); };
}

void IOSAudioRecorder::runSideEffects(const AudioBufferList *inputBuffer, int numFrames)
{
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
      const size_t channelCount = std::min(
          adapterNode_->getChannelCount(), static_cast<size_t>(inputBuffer->mNumberBuffers));
      for (size_t channel = 0; channel < channelCount; ++channel) {
        auto *data = static_cast<float *>(inputBuffer->mBuffers[channel].mData);
        adapterNode_->buff_[channel]->write(data, numFrames);
      }
    }
  }
}

void IOSAudioRecorder::handleInputConfigurationChange()
{
  if (isIdle()) {
    return;
  }

  BOOL formatChanged = NO;
  if (![nativeRecorder_ refreshResolvedInputFormatReturningChanged:&formatChanged]) {
    return;
  }

  if (!formatChanged) {
    if (state_.load(std::memory_order_acquire) == RecorderState::Recording) {
      [nativeRecorder_ setInputArmed:true];
    }
    return;
  }

  reprepareForLiveInput();
}

Result<NoneType, std::string> IOSAudioRecorder::reprepareForLiveInput()
{
  if (isIdle()) {
    return Result<NoneType, std::string>::Ok(None);
  }

  // those values are resolved earlier with actual correct values
  AVAudioFormat *inputFormat = [nativeRecorder_ getResolvedInputFormat];
  int maxInputBufferLength = [nativeRecorder_ getResolvedBufferSize];

  if (!hasUsableRecorderFormat(inputFormat) || maxInputBufferLength <= 0) {
    return Result<NoneType, std::string>::Err("Recorder input format is unavailable");
  }

  const bool shouldArmInput = state_.load(std::memory_order_acquire) == RecorderState::Recording;
  [nativeRecorder_ setInputArmed:false];

  if (usesFileOutput()) {
    auto fileResult = reprepareFileWriter(inputFormat, maxInputBufferLength);
    if (fileResult.is_err()) {
      if (shouldArmInput) {
        [nativeRecorder_ setInputArmed:true];
      }
      return fileResult;
    }
  }

  if (usesCallback()) {
    auto callbackResult = reprepareCallback(inputFormat, maxInputBufferLength);
    if (callbackResult.is_err()) {
      if (shouldArmInput) {
        [nativeRecorder_ setInputArmed:true];
      }
      return callbackResult;
    }
  }

  if (isConnected() && adapterNode_ != nullptr) {
    reprepareAdapter(inputFormat, maxInputBufferLength);
  }

  if (shouldArmInput) {
    [nativeRecorder_ setInputArmed:true];
  }

  return Result<NoneType, std::string>::Ok(None);
}

Result<NoneType, std::string> IOSAudioRecorder::reprepareFileWriter(
    AVAudioFormat *inputFormat,
    int maxInputBufferLength)
{
  std::scoped_lock lock(fileWriterMutex_);

  if (fileWriter_ == nullptr) {
    return Result<NoneType, std::string>::Err("File writer is unavailable");
  }

  if (auto rotatingWriter = std::dynamic_pointer_cast<IOSRotatingFileWriter>(fileWriter_)) {
    auto result = rotatingWriter->reprepareStreamFormat(
        inputFormat, static_cast<size_t>(maxInputBufferLength));
    if (result.is_err()) {
      fileOutputConfigured_.store(false, std::memory_order_release);
      return Result<NoneType, std::string>::Err(
          "Failed to reopen file for writing: " + result.unwrap_err());
    }

    filePath_ = result.unwrap();
    fileOutputConfigured_.store(true, std::memory_order_release);
    return Result<NoneType, std::string>::Ok(None);
  }

  auto iosWriter = std::static_pointer_cast<IOSFileWriter>(fileWriter_);
  auto result =
      iosWriter->reopenForInputFormatChange(inputFormat, static_cast<size_t>(maxInputBufferLength));
  if (result.is_err()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    return Result<NoneType, std::string>::Err(
        "Failed to reopen file for writing: " + result.unwrap_err());
  }

  filePath_ = result.unwrap();
  fileOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

Result<NoneType, std::string> IOSAudioRecorder::reprepareCallback(
    AVAudioFormat *inputFormat,
    int maxInputBufferLength)
{
  std::scoped_lock lock(callbackMutex_);

  if (dataCallback_ == nullptr) {
    return Result<NoneType, std::string>::Err("Callback is unavailable");
  }

  auto result = std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
                    ->prepare(inputFormat, static_cast<size_t>(maxInputBufferLength));
  if (result.is_err()) {
    callbackOutputConfigured_.store(false, std::memory_order_release);
    return Result<NoneType, std::string>::Err("Failed to prepare callback: " + result.unwrap_err());
  }

  callbackOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

void IOSAudioRecorder::reprepareAdapter(AVAudioFormat *inputFormat, int maxInputBufferLength)
{
  std::scoped_lock lock(adapterNodeMutex_);
  if (adapterNode_ == nullptr) {
    return;
  }

  // init() is a no-op on an initialized adapter, so the old format has to be torn down first.
  adapterNode_->adapterCleanup();
  adapterNode_->init(
      static_cast<size_t>(maxInputBufferLength),
      recorderFormatChannelCount(inputFormat),
      recorderFormatSampleRate(inputFormat));
  connectedConfigured_.store(true, std::memory_order_release);
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  if (!isIdle()) {
    stop();
  }

  nativeRecorder_.onInputConfigurationChange = nil;

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

// Must be called while holding the recorder locks (start() rollback paths only).
void IOSAudioRecorder::rollbackFailedStart()
{
  [nativeRecorder_ setInputArmed:false];
  [nativeRecorder_ stop];

  if (fileWriter_ != nullptr) {
    fileWriter_->closeFile();
    fileWriter_ = nullptr;
    fileOutputConfigured_.store(false, std::memory_order_release);
    filePath_ = "";
  }

  if (connectedConfigured_.load(std::memory_order_acquire)) {
    if (adapterNode_ != nullptr) {
      adapterNode_->adapterCleanup();
    }
    connectedConfigured_.store(false, std::memory_order_release);
  }
}

/// @brief Starts the audio recording process and prepares necessary resources.
/// Invoked asynchronously from the PromiseVendor thread pool (not the JS thread);
/// all shared state is guarded by the recorder locks and atomics.
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
    rollbackFailedStart();

    std::string message = "Failed to start native recorder";
    message += ": exception={name=";
    message += [[exception.name description] UTF8String];
    message += ", reason=";
    message += [[(exception.reason ?: @"<none>") description] UTF8String];
    message += "}";

    return Result<NoneType, std::string>::Err(withStartDiagnostics(message, audioSessionManager));
  }

  if (!didStartNativeRecorder) {
    std::string message = "Failed to start native recorder";

    if (nativeStartError != nil) {
      message += ": ";
      message += [[nativeStartError debugDescription] UTF8String];
    }

    return Result<NoneType, std::string>::Err(withStartDiagnostics(message, audioSessionManager));
  }

  AVAudioFormat *inputFormat = [nativeRecorder_ getResolvedInputFormat];

  if (!hasUsableRecorderFormat(inputFormat)) {
    std::string message =
        "Audio input format is unavailable. " + describeRecorderFormat(inputFormat);

    rollbackFailedStart();
    return Result<NoneType, std::string>::Err(withStartDiagnostics(message, audioSessionManager));
  }

  // Estimate the maximum input buffer lengths that can be expected from the sink node
  size_t maxInputBufferLength = [nativeRecorder_ getResolvedBufferSize];

  if (wantsFileOutput()) {
    recordingSegmentPaths_.clear();
    auto writerResult = setupFileWriter(fileProperties_, fileNameOverride);
    if (writerResult.is_err()) {
      rollbackFailedStart();
      return Result<NoneType, std::string>::Err(writerResult.unwrap_err());
    }
    filePath_ = writerResult.unwrap();
  }

  if (wantsCallback()) {
    if (dataCallback_ == nullptr) {
      dataCallback_ = std::make_shared<IOSRecorderCallback>(
          audioEventHandlerRegistry_,
          dataCallbackMetadata_.sampleRate,
          dataCallbackMetadata_.bufferLength,
          dataCallbackMetadata_.channelCount,
          dataCallbackMetadata_.callbackId);
    }

    dataCallback_->assignOnErrorCallbackId(errorEvent().getCallbackId());
    auto callbackResult = std::static_pointer_cast<IOSRecorderCallback>(dataCallback_)
                              ->prepare(inputFormat, maxInputBufferLength);

    if (callbackResult.is_err()) {
      rollbackFailedStart();
      callbackOutputConfigured_.store(false, std::memory_order_release);
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
/// Invoked asynchronously from the PromiseVendor thread pool (not the JS thread);
/// all shared state is guarded by the recorder locks and atomics.
/// @returns Result containing paths, size, and duration if stopped successfully, or an error message.
AudioRecorder::StopResult IOSAudioRecorder::stop()
{
  std::shared_ptr<AudioFileWriter> fileWriter;
  std::shared_ptr<AudioRecorderCallback> dataCallback;
  std::shared_ptr<RecorderAdapterNode> adapterNode;
  std::vector<std::string> outputPaths;

  if (isIdle()) {
    return StopResult::Err("Recorder is not in recording state.");
  }

  state_.store(RecorderState::Idle, std::memory_order_release);
  [nativeRecorder_ setInputArmed:false];
  [nativeRecorder_ stop];

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);

    if (usesFileOutput()) {
      fileOutputConfigured_.store(false, std::memory_order_release);
      fileWriter = std::move(fileWriter_);
      outputPaths = buildOutputPaths(recordingSegmentPaths_, filePath_);
    }

    if (usesCallback()) {
      callbackOutputConfigured_.store(false, std::memory_order_release);
      dataCallback = std::move(dataCallback_);
    }

    if (isConnected()) {
      connectedConfigured_.store(false, std::memory_order_release);
      isConnected_.store(false, std::memory_order_release);
      adapterNode = std::move(adapterNode_);
    }

    recordingSegmentPaths_.clear();
    filePath_ = "";
  }

  return finalizeStoppedOutputs(fileWriter, dataCallback, adapterNode, std::move(outputPaths));
}

/// @brief Enables file output for the recorder with specified properties.
/// The file itself is created by the next start(). An active (recording or paused) session keeps
/// the output it started with, so calling this during a session fails and changes nothing.
/// This method should be called from the JS thread only.
/// @param properties Shared pointer to AudioFileProperties defining the output file format.
/// @returns Ok when the properties were stored, otherwise an error message.
Result<NoneType, std::string> IOSAudioRecorder::enableFileOutput(
    std::shared_ptr<AudioFileProperties> properties)
{
  std::scoped_lock lock(fileWriterMutex_, errorCallbackMutex_);

  if (!isIdle()) {
    return Result<NoneType, std::string>::Err(
        "File output cannot be changed while a recording session is active");
  }

  fileProperties_ = properties;
  fileOutputEnabled_.store(true, std::memory_order_release);
  fileOutputConfigured_.store(false, std::memory_order_release);

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

  fileWriter_->assignOnErrorCallbackId(errorEvent().getCallbackId());

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

  state_.store(RecorderState::Recording, std::memory_order_release);

  if (![nativeRecorder_ resume]) {
    state_.store(RecorderState::Paused, std::memory_order_release);
  }
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

  dataCallbackMetadata_ = {
      .sampleRate = sampleRate,
      .bufferLength = bufferLength,
      .channelCount = channelCount,
      .callbackId = callbackId};

  dataCallback_ = std::make_shared<IOSRecorderCallback>(
      audioEventHandlerRegistry_, sampleRate, bufferLength, channelCount, callbackId);
  dataCallback_->setOnErrorCallback(errorEvent().getCallbackId());
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

} // namespace audioapi
