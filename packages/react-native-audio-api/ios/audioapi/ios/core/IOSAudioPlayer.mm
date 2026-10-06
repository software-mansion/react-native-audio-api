#import <AVFoundation/AVFoundation.h>
#include <audioapi/core/AudioPlayer.h>
#include <audioapi/utils/Macros.h>

#include <algorithm>
#include <cstring>

#include <audioapi/core/AudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/CurrentRenderScope.h>
#include <audioapi/ios/core/IOSAudioPlayer.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/ios/system/AudioSessionManager.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <mutex>
#include <utility>

namespace audioapi {

namespace {

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

} // namespace

IOSAudioPlayer::IOSAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    int channelCount,
    std::atomic<uint32_t> &currentRenders,
    std::weak_ptr<AudioContext> context,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint)
    : AudioPlayer(
          renderAudio,
          sampleRate,
          channelCount,
          currentRenders,
          std::move(context),
          driverMutex,
          latencyHint)
{
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  cleanup();
}

NativeAudioPlayer *IOSAudioPlayer::createNativePlayer()
{
  RenderAudioBlock renderAudioBlock = ^(AudioBufferList *outputData, int numFrames) {
    deliverOutputBuffers(outputData, numFrames);
  };

  NativeAudioPlayer *nativePlayer =
      [[NativeAudioPlayer alloc] initWithRenderAudio:renderAudioBlock
                                          sampleRate:sampleRate_
                                        channelCount:channelCount_
                             preferredIOBufferFrames:preferredIOBufferFramesFor(latencyHint_)];

  std::mutex *driverMutexForCallback = driverMutex_;
  std::weak_ptr<AudioContext> weakContext = context_;
  IOSAudioPlayer *player = this;
  nativePlayer.onStreamFail = ^{
    // Called only once the context, which owns this player, is locked alive.
    reportStreamFailToContext(driverMutexForCallback, weakContext, [player] {
      return player->isRunning_.load(std::memory_order_acquire) && !player->isRunning();
    });
  };

  return nativePlayer;
}
void IOSAudioPlayer::clearPendingSaved()
{
  pendingSavedCount_ = 0;
  pendingSaved_.zero();
}

void IOSAudioPlayer::deliverOutputBuffers(AudioBufferList *outputData, int numFrames)
{
  const CurrentRenderScope renderScope(currentRenders_);

  // If requested, clear any saved overflow before continuing normal rendering.
  if (flushOverflowNextPull_.exchange(false, std::memory_order_acq_rel)) {
    clearPendingSaved();
  }

  // if not running, set output to 0
  if (!isRunning_.load(std::memory_order_acquire)) {
    for (int channel = 0; channel < channelCount_; ++channel) {
      auto *outputChannel = static_cast<float *>(outputData->mBuffers[channel].mData);
      std::memset(outputChannel, 0, static_cast<size_t>(numFrames) * sizeof(float));
    }
    return;
  }

  int outPos = 0;
  while (outPos < numFrames) {
    const int need = numFrames - outPos;

    if (pendingSavedCount_ > 0) {
      const int fromPending = std::min(need, pendingSavedCount_);

      // populate output with pendingSaved
      for (int ch = 0; ch < channelCount_; ++ch) {
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
      for (int ch = 0; ch < channelCount_; ++ch) {
        auto *src = (*renderBuffer_)[ch].begin();
        float *dst = static_cast<float *>(outputData->mBuffers[ch].mData) + outPos;
        std::memcpy(dst, src, RENDER_QUANTUM_SIZE * sizeof(float));
      }
      outPos += RENDER_QUANTUM_SIZE;
    } else {
      // when output will be sliced, copy the remaining frames to pendingSaved
      const int tail = RENDER_QUANTUM_SIZE - stillNeed;
      for (int ch = 0; ch < channelCount_; ++ch) {
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

  bool success = [audioPlayer_ start];
  if (success) {
    flushOverflowNextPull_.store(true, std::memory_order_release);
  }
  isRunning_.store(success, std::memory_order_release);
  return success;
}

void IOSAudioPlayer::stop()
{
  isRunning_.store(false, std::memory_order_release);
  [audioPlayer_ stop];
}

bool IOSAudioPlayer::resume()
{
  if (isRunning()) {
    return true;
  }

  bool success = [audioPlayer_ resume];
  if (success) {
    flushOverflowNextPull_.store(true, std::memory_order_release);
  }
  isRunning_.store(success, std::memory_order_release);
  return success;
}

void IOSAudioPlayer::suspend()
{
  isRunning_.store(false, std::memory_order_release);
  [audioPlayer_ suspend];
}

bool IOSAudioPlayer::isRunning() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];

  return isRunning_.load(std::memory_order_acquire) && [audioEngine isEngineRunning] &&
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
}

void IOSAudioPlayer::cleanup()
{
  stop();
  [audioPlayer_ cleanup];
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

} // namespace audioapi
