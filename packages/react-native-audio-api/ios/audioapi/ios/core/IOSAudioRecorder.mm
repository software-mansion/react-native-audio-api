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
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
#include <audioapi/ios/core/utils/IOSInterleaving.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <audioapi/utils/Result.hpp>

namespace audioapi {

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

/// JS thread only. Only the iOS fields of @p options are read. Buffers are sized in start().
IOSAudioRecorder::IOSAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const AudioRecorderOptions &options)
    : AudioRecorder(audioEventHandlerRegistry)
{
  AudioReceiverBlock receiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames) {
    // The mic delivers planar float32; every consumer takes interleaved.
    const float *interleaved = ios_interleaving::interleaveAudioInput(
        inputBuffer, numFrames, inputChannelCount_, interleavedHolder_);
    if (interleaved == nullptr) {
      return;
    }

    onAudioFrames(interleaved, numFrames);
  };

  nativeRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:receiverBlock
                                                voiceProcessingEnabled:options.iosVoiceProcessing];

  nativeRecorder_.onInputConfigurationChange = ^{ this->handleInputConfigurationChange(); };
}

Result<AudioRecorder::StreamFormat, std::string> IOSAudioRecorder::resolveStreamFormat() const
{
  AVAudioFormat *inputFormat = [nativeRecorder_ getResolvedInputFormat];
  const int maxFramesPerBuffer = [nativeRecorder_ getResolvedBufferSize];

  if (!hasUsableRecorderFormat(inputFormat) || maxFramesPerBuffer <= 0) {
    return Result<StreamFormat, std::string>::Err("recorder input format is unavailable");
  }

  return Result<StreamFormat, std::string>::Ok(
      StreamFormat{
          .sampleRate = static_cast<float>(inputFormat.sampleRate),
          .channelCount = static_cast<int32_t>(inputFormat.channelCount),
          .maxFramesPerBuffer = maxFramesPerBuffer});
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

  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return Result<NoneType, std::string>::Err("Recorder input format is unavailable");
  }

  const auto format = formatResult.unwrap();
  const bool shouldArmInput = state_.load(std::memory_order_acquire) == RecorderState::Recording;
  [nativeRecorder_ setInputArmed:false];

  // Safe only because the input is now disarmed: the audio thread reads these unlocked.
  // Must happen before any early return below, or a channel-count change would make
  // interleaveAudioInput() drop every buffer once the input is re-armed.
  inputChannelCount_ = format.channelCount;
  interleavedHolder_.assign(
      static_cast<size_t>(format.maxFramesPerBuffer) * static_cast<size_t>(inputChannelCount_),
      0.0F);

  if (usesFileOutput()) {
    auto fileResult = reprepareFileWriter(format);
    if (fileResult.is_err()) {
      if (shouldArmInput) {
        [nativeRecorder_ setInputArmed:true];
      }
      return fileResult;
    }
  }

  if (usesCallback()) {
    auto callbackResult = reprepareCallback(format);
    if (callbackResult.is_err()) {
      if (shouldArmInput) {
        [nativeRecorder_ setInputArmed:true];
      }
      return callbackResult;
    }
  }

  if (isConnected()) {
    std::scoped_lock adapterLock(adapterNodeMutex_);
    // init() is a no-op on an initialized adapter, so the old format has to be torn down first.
    if (adapterNodeHandle_ != nullptr) {
      static_cast<RecorderAdapterNode *>(adapterNodeHandle_->audioNode.get())->adapterCleanup();
    }
    prepareAdapterNode(format);
  }

  streamSampleRate_.store(format.sampleRate, std::memory_order_release);

  if (shouldArmInput) {
    [nativeRecorder_ setInputArmed:true];
  }

  return Result<NoneType, std::string>::Ok(None);
}

Result<NoneType, std::string> IOSAudioRecorder::reprepareFileWriter(const StreamFormat &format)
{
  std::scoped_lock lock(fileWriterMutex_);

  if (fileWriter_ == nullptr) {
    return Result<NoneType, std::string>::Err("File writer is unavailable");
  }

  // The file stays the same; only the encoder's input side follows the new format.
  auto result = fileWriter_->reprepareStreamFormat(
      format.sampleRate, format.channelCount, format.maxFramesPerBuffer);
  if (result.is_err()) {
    fileOutputConfigured_.store(false, std::memory_order_release);
    return Result<NoneType, std::string>::Err(
        "Failed to continue the recording in the new input format: " + result.unwrap_err());
  }

  filePath_ = result.unwrap();
  fileOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

Result<NoneType, std::string> IOSAudioRecorder::reprepareCallback(const StreamFormat &format)
{
  std::scoped_lock lock(callbackMutex_);

  if (dataCallback_ == nullptr) {
    return Result<NoneType, std::string>::Err("Callback is unavailable");
  }

  auto result = dataCallback_->prepare(
      format.sampleRate, format.channelCount, static_cast<size_t>(format.maxFramesPerBuffer));
  if (result.is_err()) {
    callbackOutputConfigured_.store(false, std::memory_order_release);
    return Result<NoneType, std::string>::Err("Failed to prepare callback: " + result.unwrap_err());
  }

  callbackOutputConfigured_.store(true, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  stop();

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
    adapterNodeHandle_ = nullptr;
  }

  [nativeRecorder_ cleanup];
}

/// JS thread only.
Result<NoneType, std::string> IOSAudioRecorder::start()
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

  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    std::string message = "Audio input format is unavailable. " +
        describeRecorderFormat([nativeRecorder_ getResolvedInputFormat]) + "; " +
        [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];
#if TARGET_OS_SIMULATOR
    message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

    cleanupStartedRecorder(nativeRecorder_, fileWriter_, false);
    return Result<NoneType, std::string>::Err(message);
  }

  const auto streamFormat = formatResult.unwrap();
  const auto maxInputBufferLength = static_cast<size_t>(streamFormat.maxFramesPerBuffer);
  streamSampleRate_.store(streamFormat.sampleRate, std::memory_order_release);

  // The audio thread reads these before taking any consumer mutex, so they may only be
  // touched while the input is disarmed — i.e. here and in stop().
  inputChannelCount_ = streamFormat.channelCount;
  interleavedHolder_.assign(maxInputBufferLength * static_cast<size_t>(inputChannelCount_), 0.0F);
  lastCallbackFrameCount_.store(0, std::memory_order_release);
  bool fileWasOpened = false;

  if (wantsFileOutput()) {
    {
      std::scoped_lock segmentPathsLock(segmentPathsMutex_);
      recordingSegmentPaths_.clear();
    }
    auto writerResult = setupFileWriter(fileProperties_);
    if (!writerResult.is_ok()) {
      cleanupStartedRecorder(nativeRecorder_, fileWriter_, false);
      return Result<NoneType, std::string>::Err(writerResult.unwrap_err());
    }
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
    auto callbackResult = dataCallback_->prepare(
        streamFormat.sampleRate, streamFormat.channelCount, maxInputBufferLength);

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

  if (wantsConnection()) {
    prepareAdapterNode(streamFormat);
  }

  [nativeRecorder_ setInputArmed:true];
  state_.store(RecorderState::Recording, std::memory_order_release);
  return Result<NoneType, std::string>::Ok(None);
}

/// JS thread only.
AudioRecorder::StopResult IOSAudioRecorder::stop()
{
  DetachedSideEffects sideEffects;

  if (isIdle()) {
    return StopResult::Err("Recorder is not in recording state.");
  }

  state_.store(RecorderState::Idle, std::memory_order_release);
  [nativeRecorder_ setInputArmed:false];
  interleavedHolder_.clear();
  inputChannelCount_ = 0;
  lastCallbackFrameCount_.store(0, std::memory_order_release);
  streamSampleRate_.store(0.0F, std::memory_order_release);
  [nativeRecorder_ stop];

  {
    std::scoped_lock stopLock(callbackMutex_, fileWriterMutex_, adapterNodeMutex_);
    sideEffects = detachSideEffects();
  }

  return finalizeSideEffects(std::move(sideEffects));
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

/// Any thread. Also requires the audio engine to be running, so a recorder whose session was
/// interrupted reads as not recording.
bool IOSAudioRecorder::isRecording() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  return state_.load(std::memory_order_acquire) == RecorderState::Recording &&
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
}

/// Any thread. A stopped audio engine reads as paused, so an interrupted session looks paused.
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

/// Any thread.
bool IOSAudioRecorder::isIdle() const
{
  return state_.load(std::memory_order_acquire) == RecorderState::Idle;
}

double IOSAudioRecorder::getInputLatency() const
{
  if (isIdle()) {
    return 0.0;
  }

  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];

  double baseLatency = 0.0;
  const int32_t callbackFrames = lastCallbackFrameCount_.load(std::memory_order_acquire);
  const auto sampleRate = streamSampleRate_.load(std::memory_order_acquire);
  if (callbackFrames > 0 && sampleRate > 0.0F) {
    baseLatency = static_cast<double>(callbackFrames) / static_cast<double>(sampleRate);
  } else {
    baseLatency = [sessionManager ioBufferDurationSeconds];
  }

  return baseLatency + [sessionManager inputLatencySeconds];
}

} // namespace audioapi
