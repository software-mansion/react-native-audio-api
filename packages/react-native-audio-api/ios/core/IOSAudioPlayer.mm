#import <AVFoundation/AVFoundation.h>

#include <AudioArray.h>
#include <AudioBus.h>
#include <Constants.h>
#include <IOSAudioPlayer.h>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(const std::function<void(AudioBus *, int)> &renderAudio)
    : renderAudio_(renderAudio), audioBus_(0)
{
  RenderAudioBlock renderAudioBlock = ^(AudioBufferList *outputData, int numFrames) {
    int processedFrames = 0;

    while (processedFrames < numFrames) {
      int framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);
      renderAudio_(audioBus_, framesToProcess);

      // TODO: optimize this with SIMD?
      for (int channel = 0; channel < CHANNEL_COUNT; channel += 1) {
        float *outputChannel = (float *)outputData->mBuffers[channel].mData;
        auto *inputChannel = audioBus_->getChannel(channel)->getData();

        for (int i = 0; i < framesToProcess; i++) {
          outputChannel[processedFrames + i] = inputChannel[i];
        }
      }

      processedFrames += framesToProcess;
    }
  };

  audioPlayer_ = [[AudioPlayer alloc] initWithRenderAudioBlock:renderAudioBlock];
  audioBus_ = new AudioBus([audioPlayer_ getSampleRate], RENDER_QUANTUM_SIZE, CHANNEL_COUNT);
}

IOSAudioPlayer::IOSAudioPlayer(const std::function<void(AudioBus *, int)> &renderAudio, int sampleRate)
    : renderAudio_(renderAudio), audioBus_(0)
{
  RenderAudioBlock renderAudioBlock = ^(AudioBufferList *outputData, int numFrames) {
    int processedFrames = 0;

    while (processedFrames < numFrames) {
      int framesToProcess = std::min(numFrames - processedFrames, RENDER_QUANTUM_SIZE);
      renderAudio_(audioBus_, framesToProcess);

      // TODO: optimize this with SIMD?
      for (int channel = 0; channel < CHANNEL_COUNT; channel += 1) {
        float *outputChannel = (float *)outputData->mBuffers[channel].mData;
        auto *inputChannel = audioBus_->getChannel(channel)->getData();

        for (int i = 0; i < framesToProcess; i++) {
          outputChannel[processedFrames + i] = inputChannel[i];
        }
      }

      processedFrames += framesToProcess;
    }
  };

  audioPlayer_ = [[AudioPlayer alloc] initWithRenderAudioBlock:renderAudioBlock sampleRate:sampleRate];
  audioBus_ = new AudioBus([audioPlayer_ getSampleRate], RENDER_QUANTUM_SIZE, CHANNEL_COUNT);
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  stop();
  [audioPlayer_ cleanup];

  if (audioBus_) {
    delete audioBus_;
    audioBus_ = 0;
  }
}

void IOSAudioPlayer::start()
{
  return [audioPlayer_ start];
}

void IOSAudioPlayer::stop()
{
  return [audioPlayer_ stop];
}

int IOSAudioPlayer::getSampleRate() const
{
  return [audioPlayer_ getSampleRate];
}

} // namespace audioapi
