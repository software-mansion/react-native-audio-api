#import <AVFoundation/AVFoundation.h>

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/IOSAudioPlayer.h>
#include <audioapi/ios/system/AudioEngine.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(
    const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio,
    float sampleRate,
    int channelCount)
    : renderAudio_(renderAudio), channelCount_(channelCount), audioBus_(0), isRunning_(false)
{
  RenderAudioBlock renderAudioBlock = ^(AudioBufferList *outputData, int numFrames) {
    int processedFrames = 0;

    while (processedFrames < numFrames) {
      int framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);

      if (isRunning_.load()) {
        renderAudio_(audioBus_, framesToProcess);
      } else {
        audioBus_->zero();
      }

      for (int channel = 0; channel < channelCount_; channel += 1) {
        float *outputChannel = (float *)outputData->mBuffers[channel].mData;
        auto *inputChannel = audioBus_->getChannel(channel)->getData();

        memcpy(outputChannel + processedFrames, inputChannel, framesToProcess * sizeof(float));
      }

      processedFrames += framesToProcess;
    }
  };

  audioPlayer_ = [[NativeAudioPlayer alloc] initWithRenderAudio:renderAudioBlock
                                                     sampleRate:sampleRate
                                                   channelCount:channelCount_];

  audioBus_ = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate);
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
  isRunning_.store(success);
  return success;
}

void IOSAudioPlayer::stop()
{
  isRunning_.store(false);
  [audioPlayer_ stop];
}

bool IOSAudioPlayer::resume()
{
  if (isRunning()) {
    return true;
  }

  bool success = [audioPlayer_ resume];
  isRunning_.store(success);
  return success;
}

void IOSAudioPlayer::suspend()
{
  isRunning_.store(false);
  [audioPlayer_ suspend];
}

bool IOSAudioPlayer::isRunning() const
{
  AudioEngine *audioEngine = [AudioEngine sharedInstance];

  return isRunning_.load() && [audioEngine isRunning];
}

void IOSAudioPlayer::cleanup()
{
  stop();
  [audioPlayer_ cleanup];

  if (audioBus_) {
    audioBus_ = nullptr;
  }
}

} // namespace audioapi
