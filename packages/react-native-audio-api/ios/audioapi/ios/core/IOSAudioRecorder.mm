#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <audioapi/ios/system/AudioAPIError.h>
#import <audioapi/ios/system/AudioSessionManager.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/ios/core/IOSAudioRecorder.h>
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

/// Returns false — dropping the buffer — when the layout no longer
/// matches what we were configured with, which happens if the input node is rebuilt on a route
/// change while a recording is in flight.
static bool collectPlanarInputChannels(
    const AudioBufferList *input,
    int numFrames,
    int channelCount,
    const float **channels)
{
  if (input == nullptr || numFrames <= 0 || channelCount <= 0 || channelCount > MAX_CHANNEL_COUNT ||
      input->mNumberBuffers != static_cast<UInt32>(channelCount)) {
    return false;
  }
  const size_t frameBytes = static_cast<size_t>(numFrames) * sizeof(float);
  for (int channel = 0; channel < channelCount; ++channel) {
    if (input->mBuffers[channel].mDataByteSize < frameBytes) {
      return false;
    }
    channels[channel] = static_cast<const float *>(input->mBuffers[channel].mData);
  }
  return true;
}

static void stopStartedRecorder(NativeAudioRecorder *nativeRecorder)
{
  [nativeRecorder setInputArmed:false];
  [nativeRecorder stop];
}

/// JS thread only. Only the iOS fields of @p options are read. Buffers are sized in start().
IOSAudioRecorder::IOSAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const AudioRecorderOptions &options)
    : AudioRecorder(audioEventHandlerRegistry)
{
  AudioReceiverBlock receiverBlock = ^(const AudioBufferList *inputBuffer, int numFrames) {
    const float *channels[MAX_CHANNEL_COUNT];
    if (!collectPlanarInputChannels(inputBuffer, numFrames, inputChannelCount_, channels)) {
      return;
    }

    onAudioFrames(channels, numFrames);
  };

  nativeRecorder_ = [[NativeAudioRecorder alloc] initWithReceiverBlock:receiverBlock
                                                voiceProcessingEnabled:options.iosVoiceProcessing];

  nativeRecorder_.onInputNotification =
      ^(AudioEngineInputNotification notification, NSError *error) {
        this->handleInputNotification(notification, error);
      };
}

Result<StreamFormat, std::string> IOSAudioRecorder::resolveStreamFormat() const
{
  AVAudioFormat *inputFormat = [nativeRecorder_ getResolvedInputFormat];
  const int maxFramesPerBuffer = [nativeRecorder_ getResolvedBufferSize];

  if (!hasUsableRecorderFormat(inputFormat) || maxFramesPerBuffer <= 0) {
    return Err("recorder input format is unavailable");
  }

  return Ok(
      StreamFormat{
          .layout =
              {
                  .sampleRate = static_cast<float>(inputFormat.sampleRate),
                  .channelCount = static_cast<int>(inputFormat.channelCount),
              },
          .maxFramesPerBuffer = static_cast<size_t>(maxFramesPerBuffer)});
}

void IOSAudioRecorder::handleInputNotification(
    AudioEngineInputNotification notification,
    NSError *error)
{
  switch (notification) {
    case AudioEngineInputNotificationHardwareChanged:
      handleHardwareChange();
      return;
    case AudioEngineInputNotificationRecoveryFailed:
      reportError(
          std::string("iOS recorder error: failed to recover the input: ") +
          [[AudioAPIError describe:error] UTF8String]);
      return;
  }
}

void IOSAudioRecorder::handleHardwareChange()
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

  auto result = reprepareForLiveInput();
  if (result.is_err()) {
    reportError("iOS recorder error: " + result.unwrap_err());
  }
}

Result<NoneType, std::string> IOSAudioRecorder::reprepareForLiveInput()
{
  if (isIdle()) {
    return Ok(None);
  }

  auto formatResult = resolveStreamFormat();

  if (!formatResult.is_ok()) {
    return Err("Recorder input format is unavailable");
  }

  const auto format = formatResult.unwrap();
  const bool shouldArmInput = state_.load(std::memory_order_acquire) == RecorderState::Recording;
  [nativeRecorder_ setInputArmed:false];

  // Safe only because the input is now disarmed: the audio thread reads this unlocked.
  // Must happen before any early return below, or a channel-count change would make
  // collectPlanarInputChannels() drop every buffer once the input is re-armed.
  inputChannelCount_ = format.layout.channelCount;

  Result<NoneType, std::string> outputsResult = Ok(None);
  {
    std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
    outputsResult = reprepareOutputs(format);
  }

  streamSampleRate_.store(format.layout.sampleRate, std::memory_order_release);

  if (shouldArmInput) {
    [nativeRecorder_ setInputArmed:true];
  }

  return outputsResult;
}

IOSAudioRecorder::~IOSAudioRecorder()
{
  stop();

  nativeRecorder_.onInputNotification = nil;
  [nativeRecorder_ cleanup];
}

/// Control lane only (the recorder HostObject's single-worker promise vendor).
Result<NoneType, std::string> IOSAudioRecorder::start()
{
  if (!isIdle()) {
    return Err("Recorder is already recording");
  }

  Result<NoneType, std::string> outputsResult = Ok(None);
  DetachedSideEffects abandoned;

  {
    std::scoped_lock startLock(callback_.mutex, file_.mutex, adapter_.mutex);
    AudioSessionManager *audioSessionManager = [AudioSessionManager sharedInstance];

    if ([[audioSessionManager checkRecordingPermissions] isEqual:@"Denied"]) {
      return Err("Microphone permissions are not granted");
    }

    NSError *nativeStartError = nil;
    BOOL didStartNativeRecorder = NO;

    @try {
      didStartNativeRecorder = [nativeRecorder_ start:&nativeStartError];
    } @catch (NSException *exception) {
      stopStartedRecorder(nativeRecorder_);

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

      return Err(message);
    }

    if (!didStartNativeRecorder) {
      auto message = std::string("Failed to start native recorder: ") +
          [[AudioAPIError describe:nativeStartError] UTF8String];
      message += "; ";
      message += [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];

#if TARGET_OS_SIMULATOR
      message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

      return Err(message);
    }

    auto formatResult = resolveStreamFormat();

    if (!formatResult.is_ok()) {
      std::string message = "Audio input format is unavailable. " +
          describeRecorderFormat([nativeRecorder_ getResolvedInputFormat]) + "; " +
          [[audioSessionManager inputDiagnosticsSnapshot] UTF8String];
#if TARGET_OS_SIMULATOR
      message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif

      stopStartedRecorder(nativeRecorder_);
      return Err(message);
    }

    const auto streamFormat = formatResult.unwrap();
    streamSampleRate_.store(streamFormat.layout.sampleRate, std::memory_order_release);

    // The audio thread reads these before taking any consumer mutex, so they may only be
    // touched while the input is disarmed — i.e. here and in stop().
    inputChannelCount_ = streamFormat.layout.channelCount;
    lastCallbackFrameCount_.store(0, std::memory_order_release);

    outputsResult = prepareOutputs(streamFormat);

    if (outputsResult.is_ok()) {
      [nativeRecorder_ setInputArmed:true];
      state_.store(RecorderState::Recording, std::memory_order_release);
      return Ok(None);
    }

    stopStartedRecorder(nativeRecorder_);
    abandoned = detachSideEffects();
  }

  // Closes the outputs prepared before the one that failed, with no recorder mutex held.
  finalizeSideEffects(std::move(abandoned));
  return outputsResult;
}

/// Control lane, or any thread holding the ActiveRecorderHandle mutex.
Result<FileInfo, std::string> IOSAudioRecorder::stop()
{
  DetachedSideEffects sideEffects;

  if (isIdle()) {
    return Err("Recorder is not in recording state.");
  }

  state_.store(RecorderState::Idle, std::memory_order_release);
  [nativeRecorder_ setInputArmed:false];
  inputChannelCount_ = 0;
  lastCallbackFrameCount_.store(0, std::memory_order_release);
  streamSampleRate_.store(0.0F, std::memory_order_release);
  [nativeRecorder_ stop];

  {
    std::scoped_lock stopLock(callback_.mutex, file_.mutex, adapter_.mutex);
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

Result<NoneType, std::string> IOSAudioRecorder::rerouteInput()
{
  return Result<NoneType, std::string>::Ok(None);
}

Result<NoneType, std::string> IOSAudioRecorder::resume()
{
  if (!isPaused()) {
    return Ok(None);
  }

  NSError *resumeError = nil;
  if (![nativeRecorder_ resume:&resumeError]) {
    return Err(
        std::string("Failed to resume native recorder: ") +
        [[AudioAPIError describe:resumeError] UTF8String]);
  }

  state_.store(RecorderState::Recording, std::memory_order_release);
  handleHardwareChange();
  return Ok(None);
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

double IOSAudioRecorder::platformInputLatency(double callbackSeconds) const
{
  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  const double bufferLatency =
      callbackSeconds > 0.0 ? callbackSeconds : [sessionManager ioBufferDurationSeconds];
  return bufferLatency + [sessionManager inputLatencySeconds];
}

std::shared_ptr<AudioRecorder> createPlatformAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const AudioRecorderOptions &options)
{
  return std::make_shared<IOSAudioRecorder>(audioEventHandlerRegistry, options);
}

} // namespace audioapi
