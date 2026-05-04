#import <AVFoundation/AVFoundation.h>

#include <algorithm>
#include <cstring>

#include <audioapi/core/utils/Constants.h>
#include <audioapi/ios/core/IOSAudioPlayer.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(
    const std::function<void(std::shared_ptr<DSPAudioBuffer>, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : audioBuffer_(nullptr),
      audioPlayer_(nullptr),
      renderAudio_(renderAudio),
      channelCount_(channelCount),
      isRunning_(false),
      pendingSaved_(RENDER_QUANTUM_SIZE, channelCount_, sampleRate)
{
  RenderAudioBlock renderAudioBlock = ^(AudioBufferList *outputData, int numFrames) {
    deliverOutputBuffers(outputData, numFrames);
  };

  audioPlayer_ = [[NativeAudioPlayer alloc] initWithRenderAudio:renderAudioBlock
                                                     sampleRate:sampleRate
                                                   channelCount:channelCount_];
  audioBuffer_ = std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate);
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

void IOSAudioPlayer::deliverOutputBuffers(AudioBufferList *outputData, int numFrames)
{
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

    renderAudio_(audioBuffer_, RENDER_QUANTUM_SIZE);

    // normal rendering - take RENDER_QUANTUM_SIZE frames from the graph and copy to output
    const int stillNeed = numFrames - outPos;
    if (stillNeed >= RENDER_QUANTUM_SIZE) {
      for (int ch = 0; ch < channelCount_; ++ch) {
        auto *src = (*audioBuffer_)[ch].begin();
        float *dst = static_cast<float *>(outputData->mBuffers[ch].mData) + outPos;
        std::memcpy(dst, src, RENDER_QUANTUM_SIZE * sizeof(float));
      }
      outPos += RENDER_QUANTUM_SIZE;
    } else {
      // when output will be sliced, copy the remaining frames to pendingSaved
      const int tail = RENDER_QUANTUM_SIZE - stillNeed;
      for (int ch = 0; ch < channelCount_; ++ch) {
        auto *src = (*audioBuffer_)[ch].begin();
        float *dst = static_cast<float *>(outputData->mBuffers[ch].mData) + outPos;
        std::memcpy(dst, src, stillNeed * sizeof(float));
      }
      pendingSaved_.copy(*audioBuffer_, stillNeed, 0, tail);
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
  audioBuffer_ = nullptr;
}

} // namespace audioapi
