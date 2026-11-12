#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#import <audioapi/ios/system/AudioAPIError.h>
#import <audioapi/ios/system/AudioEngine.h>
#import <audioapi/ios/system/AudioSessionManager.h>

#include <audioapi/core/inputs/RecorderInputGate.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/ios/core/IOSRecorderInput.h>
#include <audioapi/utils/AudioRecorderOptions.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

namespace {

bool hasUsableFormat(AVAudioFormat *format)
{
  return format != nil && format.sampleRate > 0 && format.channelCount > 0;
}

std::string describeFormat(AVAudioFormat *format)
{
  return "engineFormat={sampleRate=" + std::to_string(format.sampleRate) +
      ", channelCount=" + std::to_string(format.channelCount) +
      ", interleaved=" + (format.interleaved ? "true" : "false") + "}";
}

std::string withInputDiagnostics(std::string message)
{
  message += "; ";
  message += [[[AudioSessionManager sharedInstance] inputDiagnosticsSnapshot] UTF8String];
#if TARGET_OS_SIMULATOR
  message += "; simulatorHint={Select a host microphone in Simulator > I/O > Audio Input}";
#endif
  return message;
}

/// The largest buffer the engine delivers at @p sampleRate.
size_t maxFramesPerBuffer(double sampleRate)
{
  const double bufferDuration = std::max([AVAudioSession sharedInstance].IOBufferDuration, 0.2);
  return std::bit_ceil(static_cast<uint32_t>(std::ceil(bufferDuration * sampleRate)));
}

/// One buffer per channel. An interleaved layout delivers a single buffer, which the recorder then
/// drops as not matching its channel count.
bool collectChannels(
    const AudioBufferList *input,
    int numFrames,
    std::array<const float *, MAX_CHANNEL_COUNT> &channels,
    int &channelCount)
{
  if (input == nullptr || numFrames <= 0 || input->mNumberBuffers == 0 ||
      input->mNumberBuffers > static_cast<UInt32>(MAX_CHANNEL_COUNT)) {
    return false;
  }

  const size_t frameBytes = static_cast<size_t>(numFrames) * sizeof(float);
  for (UInt32 channel = 0; channel < input->mNumberBuffers; ++channel) {
    if (input->mBuffers[channel].mDataByteSize < frameBytes) {
      return false;
    }
    channels[channel] = static_cast<const float *>(input->mBuffers[channel].mData);
  }

  channelCount = static_cast<int>(input->mNumberBuffers);
  return true;
}

/// Marks the calling thread as inside an AudioEngine call made on the recorder's behalf.
class EngineCallScope {
 public:
  explicit EngineCallScope(std::atomic<std::thread::id> &engineCallThread)
      : engineCallThread_(engineCallThread)
  {
    engineCallThread_.store(std::this_thread::get_id(), std::memory_order_release);
  }

  ~EngineCallScope()
  {
    engineCallThread_.store(std::thread::id{}, std::memory_order_release);
  }

  DELETE_COPY_AND_MOVE(EngineCallScope);

 private:
  std::atomic<std::thread::id> &engineCallThread_;
};

} // namespace

IOSRecorderInput::IOSRecorderInput(const AudioRecorderOptions &options)
    : voiceProcessingEnabled_(options.iosVoiceProcessing)
{
}

IOSRecorderInput::~IOSRecorderInput()
{
  close();
}

Result<StreamFormat, std::string> IOSRecorderInput::open(
    RouteLease /*route*/,
    InputOpenPurpose /*purpose*/,
    std::shared_ptr<RecorderInputGate> gate,
    RecorderInputEvents events)
{
  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  if ([[sessionManager checkRecordingPermissions] isEqual:@"Denied"]) {
    return Err("Microphone permissions are not granted");
  }

  // Captured by value: the engine keeps both blocks until the input node is detached, possibly
  // after this input is gone.
  std::weak_ptr<IOSRecorderInput> weakSelf = weak_from_this();

  AVAudioSinkNodeReceiverBlock receiverBlock = ^OSStatus(
      const AudioTimeStamp *, AVAudioFrameCount frameCount, const AudioBufferList *input) {
    std::array<const float *, MAX_CHANNEL_COUNT> channels;
    int channelCount = 0;
    if (collectChannels(input, static_cast<int>(frameCount), channels, channelCount)) {
      gate->deliver(channels.data(), channelCount, static_cast<int>(frameCount));
    }
    return noErr;
  };

  AudioEngineInputNotificationBlock onInputNotification =
      ^(AudioEngineInputNotification notification, NSError *error) {
        auto input = weakSelf.lock();
        if (input == nullptr || input->isInsideOwnEngineCall()) {
          return;
        }

        switch (notification) {
          case AudioEngineInputNotificationHardwareChanged:
            events.formatChanged();
            return;
          case AudioEngineInputNotificationRecoveryFailed:
            events.failed([[AudioAPIError describe:error] UTF8String]);
            return;
        }
      };

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  NSError *startError = nil;
  BOOL didStart = NO;

  // The attach below stops the engine under whatever playback is running. When the engine then
  // fails to start with the input, the input is taken out again and playback started without it.
  const bool playbackWasRunning =
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
  const auto closeAndRestorePlayback = [&] {
    close();
    if (playbackWasRunning) {
      EngineCallScope engineCall(engineCallThread_);
      [audioEngine startIfNecessary:nil];
    }
  };

  // Before the first engine call, so that close() undoes whatever part of the attach went through
  // when one of them throws.
  isOpen_.store(true, std::memory_order_release);

  @try {
    EngineCallScope engineCall(engineCallThread_);
    // The engine is restarted around the attach: attaching the first input node to an engine
    // that playback already started is not allowed at runtime.
    [audioEngine stopIfNecessary];
    [audioEngine attachInputNodeWithReceiverBlock:receiverBlock
                           voiceProcessingEnabled:voiceProcessingEnabled_
                              onInputNotification:onInputNotification];
    didStart = [audioEngine startIfNecessary:&startError];
  } @catch (NSException *exception) {
    closeAndRestorePlayback();

    std::string message = "Failed to start native recorder: exception={name=";
    message += [[exception.name description] UTF8String];
    message += ", reason=";
    message += [[(exception.reason ?: @"<none>") description] UTF8String];
    message += '}';
    return Err(withInputDiagnostics(std::move(message)));
  }

  if (!didStart) {
    closeAndRestorePlayback();

    return Err(withInputDiagnostics(
        std::string("Failed to start native recorder: ") +
        [[AudioAPIError describe:startError] UTF8String]));
  }

  auto formatResult = currentFormat();
  if (formatResult.is_err()) {
    std::string message =
        "Audio input format is unavailable. " + describeFormat([audioEngine getLiveInputFormat]);
    close();
    return Err(withInputDiagnostics(std::move(message)));
  }

  // The engine passes an interleaved layout as a single buffer, which cannot be split into the
  // per-channel pointers the recorder takes, so every buffer would be dropped.
  AVAudioFormat *liveFormat = [audioEngine getLiveInputFormat];
  if (liveFormat.interleaved && liveFormat.channelCount > 1) {
    std::string message =
        "Interleaved multi-channel input is not supported. " + describeFormat(liveFormat);
    close();
    return Err(withInputDiagnostics(std::move(message)));
  }

  return formatResult;
}

Result<NoneType, std::string> IOSRecorderInput::start()
{
  return Ok(None);
}

void IOSRecorderInput::pause()
{
  [[AudioEngine sharedInstance] pauseIfNecessary];
}

Result<NoneType, std::string> IOSRecorderInput::resume()
{
  NSError *resumeError = nil;
  BOOL didResume = NO;

  {
    EngineCallScope engineCall(engineCallThread_);
    didResume = [[AudioEngine sharedInstance] startIfNecessary:&resumeError];
  }

  if (!didResume) {
    return Err(
        std::string("Failed to resume native recorder: ") +
        [[AudioAPIError describe:resumeError] UTF8String]);
  }
  return Ok(None);
}

void IOSRecorderInput::close()
{
  if (!isOpen_.exchange(false, std::memory_order_acq_rel)) {
    return;
  }

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  EngineCallScope engineCall(engineCallThread_);
  [audioEngine detachInputNode];
  [audioEngine stopIfPossible];
  [audioEngine restartAudioEngine];
}

Result<StreamFormat, std::string> IOSRecorderInput::currentFormat() const
{
  AVAudioFormat *inputFormat = [[AudioEngine sharedInstance] getLiveInputFormat];

  if (!hasUsableFormat(inputFormat)) {
    return Err("recorder input format is unavailable");
  }

  return Ok(
      StreamFormat{
          .layout =
              {
                  .sampleRate = static_cast<float>(inputFormat.sampleRate),
                  .channelCount = static_cast<int>(inputFormat.channelCount),
              },
          .maxFramesPerBuffer = maxFramesPerBuffer(inputFormat.sampleRate)});
}

/// A stopped engine (an interrupted session) is not running, so the recorder reads as paused.
bool IOSRecorderInput::isRunning() const
{
  return isOpen_.load(std::memory_order_acquire) &&
      [[AudioEngine sharedInstance] getState] == AudioEngineState::AudioEngineStateRunning;
}

double IOSRecorderInput::latency(double callbackSeconds) const
{
  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  const double bufferLatency =
      callbackSeconds > 0.0 ? callbackSeconds : [sessionManager ioBufferDurationSeconds];
  return bufferLatency + [sessionManager inputLatencySeconds];
}

bool IOSRecorderInput::isInsideOwnEngineCall() const
{
  return engineCallThread_.load(std::memory_order_acquire) == std::this_thread::get_id();
}

std::shared_ptr<IRecorderInput> createPlatformRecorderInput(const AudioRecorderOptions &options)
{
  return std::make_shared<IOSRecorderInput>(options);
}

} // namespace audioapi
