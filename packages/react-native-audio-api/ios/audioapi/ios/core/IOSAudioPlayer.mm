#import <AVFoundation/AVFoundation.h>
#include <audioapi/core/AudioPlayer.h>
#include <audioapi/utils/Macros.h>

#include <mach/mach_time.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/CurrentRenderScope.h>
#include <audioapi/ios/core/IOSAudioPlayer.h>
#include <audioapi/ios/system/AudioAPIError.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/ios/system/AudioSessionManager.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>
#include <mutex>
#include <utility>

namespace audioapi {

namespace {

const mach_timebase_info_data_t kHostTimebase = [] {
  mach_timebase_info_data_t timebase{};
  mach_timebase_info(&timebase);
  return timebase;
}();

std::int64_t machTicksToNs(std::int64_t ticks)
{
  return static_cast<std::int64_t>(
      static_cast<__int128>(ticks) * kHostTimebase.numer / kHostTimebase.denom);
}

/// In frames of the session's own rate, which may differ from the context's.
int preferredIOBufferFramesFor(AudioContextLatencyHint latencyHint)
{
  switch (latencyHint) {
    case AudioContextLatencyHint::INTERACTIVE:
      return RENDER_QUANTUM_SIZE;
    case AudioContextLatencyHint::BALANCED:
      return 8 * RENDER_QUANTUM_SIZE;
    case AudioContextLatencyHint::PLAYBACK:
      return 32 * RENDER_QUANTUM_SIZE;
  }
  return RENDER_QUANTUM_SIZE;
}

void reportStreamFailToContext(
    std::mutex *driverMutex,
    const std::weak_ptr<AudioContext> &context,
    const std::function<bool()> &isStillFailed)
{
  auto ctx = context.lock();
  if (driverMutex == nullptr || ctx == nullptr) {
    return;
  }

  std::scoped_lock lock(*driverMutex);
  if (ctx->isClosed() || !isStillFailed()) {
    return;
  }
  ctx->onStreamFail();
}

void zeroOutput(AudioBufferList *outputData, int numFrames)
{
  for (UInt32 channel = 0; channel < outputData->mNumberBuffers; ++channel) {
    std::memset(
        outputData->mBuffers[channel].mData, 0, static_cast<size_t>(numFrames) * sizeof(float));
  }
}

} // namespace

IOSAudioPlayer::IOSAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint)
    : AudioPlayer(
          renderAudio,
          sampleRate,
          currentRenders,
          std::move(context),
          driverMutex,
          latencyHint),
      preferredIOBufferFrames_(preferredIOBufferFramesFor(latencyHint))
{
  renderBuffer_ =
      std::make_unique<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, DEFAULT_CHANNEL_COUNT, sampleRate_);

  IOSAudioPlayer *player = this;
  // The engine keeps this block until the node is detached and may call it after the player is
  // gone, so it reaches the player only while its own copy of `isRunning_` says so.
  std::shared_ptr<std::atomic<bool>> isRunning = isRunning_;
  renderBlock_ = ^OSStatus(
      BOOL * /*isSilence*/,
      const AudioTimeStamp *timestamp,
      AVAudioFrameCount frameCount,
      AudioBufferList *outputData) {
    if (outputData->mNumberBuffers != DEFAULT_CHANNEL_COUNT) {
      return kAudioServicesBadPropertySizeError;
    }

    if (!isRunning->load(std::memory_order_acquire)) {
      zeroOutput(outputData, static_cast<int>(frameCount));
      return kAudioServicesNoError;
    }

    player->deliverOutputBuffers(outputData, static_cast<int>(frameCount), timestamp);
    return kAudioServicesNoError;
  };

  std::mutex *driverMutexForCallback = driverMutex_;
  std::weak_ptr<AudioContext> weakContext = context_;
  onStreamFail_ = ^{
    // Called only once the context, which owns this player, is locked alive.
    reportStreamFailToContext(driverMutexForCallback, weakContext, [player] {
      return player->isRunning_->load(std::memory_order_acquire) && !player->isRunning();
    });
  };
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  cleanup();
}

void IOSAudioPlayer::clearPendingSaved()
{
  pendingSavedCount_ = 0;
  pendingSaved_.zero();
}

void IOSAudioPlayer::deliverOutputBuffers(
    AudioBufferList *outputData,
    int numFrames,
    const AudioTimeStamp *timestamp)
{
  const CurrentRenderScope renderScope(currentRenders_);

  // If requested, clear any saved overflow before continuing normal rendering.
  if (flushOverflowNextPull_.exchange(false, std::memory_order_acq_rel)) {
    clearPendingSaved();
  }

  // if not running, set output to 0
  if (!isRunning_->load(std::memory_order_acquire)) {
    zeroOutput(outputData, numFrames);
    return;
  }

  if (timestamp != nullptr && (timestamp->mFlags & kAudioTimeStampHostTimeValid) != 0) {
    // Frames left over from the previous quantum play first; they were rendered but not delivered.
    publishOutputAnchor(
        {.contextFrame = renderedFrames_ - pendingSavedCount_,
         .hostTimeNs = machTicksToNs(static_cast<std::int64_t>(timestamp->mHostTime)),
         .isValid = true});
  }

  int outPos = 0;
  while (outPos < numFrames) {
    const int need = numFrames - outPos;

    if (pendingSavedCount_ > 0) {
      const int fromPending = std::min(need, pendingSavedCount_);

      // populate output with pendingSaved
      for (int ch = 0; ch < DEFAULT_CHANNEL_COUNT; ++ch) {
        float *dst = static_cast<float *>(outputData->mBuffers[ch].mData) + outPos;
        const float *src = pendingSaved_[ch].begin();
        std::memcpy(dst, src, fromPending * sizeof(float));

        // move the remaining samples to the beginning of the pendingSaved buffer
        const int remain = pendingSavedCount_ - fromPending;
        if (remain > 0) {
          float *buf = pendingSaved_[ch].begin();
          std::memmove(buf, buf + fromPending, remain * sizeof(float));
        }
      }

      pendingSavedCount_ -= fromPending;
      outPos += fromPending;
      continue;
    }

    renderNormalizedQuantum(RENDER_QUANTUM_SIZE);

    // normal rendering - take RENDER_QUANTUM_SIZE frames from the graph and copy to output
    const int stillNeed = numFrames - outPos;
    if (stillNeed >= RENDER_QUANTUM_SIZE) {
      for (int ch = 0; ch < DEFAULT_CHANNEL_COUNT; ++ch) {
        auto *src = (*renderBuffer_)[ch].begin();
        float *dst = static_cast<float *>(outputData->mBuffers[ch].mData) + outPos;
        std::memcpy(dst, src, RENDER_QUANTUM_SIZE * sizeof(float));
      }
      outPos += RENDER_QUANTUM_SIZE;
    } else {
      // when output will be sliced, copy the remaining frames to pendingSaved
      const int tail = RENDER_QUANTUM_SIZE - stillNeed;
      for (int ch = 0; ch < DEFAULT_CHANNEL_COUNT; ++ch) {
        auto *src = (*renderBuffer_)[ch].begin();
        float *dst = static_cast<float *>(outputData->mBuffers[ch].mData) + outPos;
        std::memcpy(dst, src, stillNeed * sizeof(float));
      }
      pendingSaved_.copy(*renderBuffer_, stillNeed, 0, tail);
      pendingSavedCount_ = tail;
      outPos += stillNeed;
    }
  }
}

bool IOSAudioPlayer::start()
{
  if (isRunning()) {
    return true;
  }

  markStarted();

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  assert(audioEngine != nil);

  // AVAudioEngine adopts a new buffer duration only when it starts, so it is requested first.
  [sessionManager setPreferredIOBufferFrames:preferredIOBufferFrames_];

  NSError *activationError = nil;
  if (![sessionManager ensureActive:false error:&activationError]) {
    [sessionManager resetPreferredIOBufferFrames];
    [AudioAPIError fail:nil with:[AudioAPIError sessionActivationFailed:activationError]];
    isRunning_->store(false, std::memory_order_release);
    return false;
  }

  // AudioEngine allows us to attach and connect nodes at runtime but with few
  // limitations in this case if it is the first player and recorder started the
  // engine we need to restart. It can be optimized by tracking if we haven't
  // break rules of at runtime modifications from docs
  // https://developer.apple.com/documentation/avfaudio/avaudioengine?language=objc
  //
  // Currently we are restarting because we do not see any significant performance issue and case when
  // you will need to start and stop player very frequently
  [audioEngine stopIfNecessary];
  if (sourceNodeId_ == nil) {
    sourceNodeId_ = [audioEngine attachSourceNodeWithRenderBlock:renderBlock_
                                                      sampleRate:sampleRate_
                                                    channelCount:DEFAULT_CHANNEL_COUNT
                                          onOutputRecoveryFailed:onStreamFail_];
  }

  if (![audioEngine startIfNecessary:nil]) {
    // Leaves no source node attached.
    stop();
    return false;
  }

  flushOverflowNextPull_.store(true, std::memory_order_release);
  isRunning_->store(true, std::memory_order_release);
  return true;
}

void IOSAudioPlayer::stop()
{
  isRunning_->store(false, std::memory_order_release);
  [[AudioSessionManager sharedInstance] resetPreferredIOBufferFrames];

  AudioEngine *audioEngine = [AudioEngine sharedInstance];
  if (audioEngine == nil) {
    return;
  }

  if (sourceNodeId_ != nil) {
    [audioEngine detachSourceNodeWithId:sourceNodeId_];
    sourceNodeId_ = nil;
  }
  [audioEngine stopIfPossible];
}

bool IOSAudioPlayer::resume()
{
  return start();
}

void IOSAudioPlayer::suspend()
{
  stop();
}

bool IOSAudioPlayer::isRunning() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];

  return isRunning_->load(std::memory_order_acquire) && [audioEngine isEngineRunning] &&
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
}

void IOSAudioPlayer::cleanup()
{
  stop();
  renderBlock_ = nil;
  onStreamFail_ = nil;
  renderBuffer_ = nullptr;
}

double IOSAudioPlayer::getBaseLatency() const
{
  if (!isRunning()) {
    return 0.0;
  }

  return static_cast<double>(RENDER_QUANTUM_SIZE) / static_cast<double>(sampleRate_);
}

double IOSAudioPlayer::getOutputLatency() const
{
  if (!isRunning()) {
    return 0.0;
  }

  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  return [sessionManager outputLatencySeconds] + [sessionManager ioBufferDurationSeconds];
}

std::optional<OutputPresentation> IOSAudioPlayer::getOutputPresentation()
{
  const auto anchor = latestOutputAnchor();
  if (!anchor) {
    return std::nullopt;
  }

  AudioSessionManager *sessionManager = [AudioSessionManager sharedInstance];
  const auto outputLatencyNs =
      static_cast<std::int64_t>(std::llround([sessionManager outputLatencySeconds] * 1e9));
  return OutputPresentation{
      .contextFrame = anchor->contextFrame,
      .presentationTimeNs = anchor->hostTimeNs + outputLatencyNs};
}

std::int64_t IOSAudioPlayer::presentationClockNowNs() const
{
  return machTicksToNs(static_cast<std::int64_t>(mach_absolute_time()));
}

std::shared_ptr<AudioPlayer> createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile /*outputProfile*/)
{
  return std::make_shared<IOSAudioPlayer>(
      renderAudio, sampleRate, currentRenders, std::move(context), driverMutex, latencyHint);
}

} // namespace audioapi
