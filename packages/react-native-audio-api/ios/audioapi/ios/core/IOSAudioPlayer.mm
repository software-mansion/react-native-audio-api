#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioPlayer.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(
    const std::function<void(std::shared_ptr<AudioBuffer>, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : renderAudio_(renderAudio), channelCount_(channelCount), audioBuffer_(0), isRunning_(false)
{
  RenderAudioBlock renderAudioBlock = ^(AudioBufferList *outputData, int numFrames) {
    int processedFrames = 0;

    while (processedFrames < numFrames) {
      int framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);

      if (isRunning_.load(std::memory_order_acquire)) {
        renderAudio_(audioBuffer_, framesToProcess);
      } else {
        audioBuffer_->zero();
      }

      for (size_t channel = 0; channel < channelCount_; channel += 1) {
        float *outputChannel = (float *)outputData->mBuffers[channel].mData;

        audioBuffer_->getChannel(channel)->copyTo(
            outputChannel, 0, processedFrames, framesToProcess);
      }

      processedFrames += framesToProcess;
    }
  };

  audioPlayer_ = [[NativeAudioPlayer alloc] initWithRenderAudio:renderAudioBlock
                                                     sampleRate:sampleRate
                                                   channelCount:channelCount_];

  audioBuffer_ = std::make_shared<AudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate);
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  cleanup();
}

bool IOSAudioPlayer::start()
{
  if (isRunning()) {
    return true;
  }

  bool success = [audioPlayer_ start];
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

  return isRunning_.load(std::memory_order_acquire) &&
      [audioEngine getState] == AudioEngineState::AudioEngineStateRunning;
}

void IOSAudioPlayer::cleanup()
{
  stop();
  [audioPlayer_ cleanup];
  audioBuffer_ = nullptr;
}

} // namespace audioapi
