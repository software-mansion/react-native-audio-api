#import <AVFoundation/AVFoundation.h>

#include <AudioArray.h>
#include <AudioBus.h>
#include <Constants.h>
#include <IOSAudioPlayer.h>
#include <IOSAudioManagerBridge.h>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(const std::shared_ptr<IOSAudioManagerBridge> audioManagerBridge, const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio)
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

  audioPlayer_ = [[AudioPlayer alloc] initWithAudioManager:audioManagerBridge->getAudioManager()  renderAudio:renderAudioBlock];
  audioBus_ = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, CHANNEL_COUNT, getSampleRate());
}

IOSAudioPlayer::IOSAudioPlayer(const std::shared_ptr<IOSAudioManagerBridge> audioManagerBridge, const std::function<void(std::shared_ptr<AudioBus>, int)> &renderAudio, float sampleRate)
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

  audioPlayer_ = [[AudioPlayer alloc] initWithAudioManager:audioManagerBridge->getAudioManager() renderAudio:renderAudioBlock sampleRate:sampleRate];
  audioBus_ = std::make_shared<AudioBus>(RENDER_QUANTUM_SIZE, CHANNEL_COUNT, getSampleRate());
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  stop();
  [audioPlayer_ cleanup];

  if (audioBus_) {
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

void IOSAudioPlayer::resume()
{
  return [audioPlayer_ resume];
}

void IOSAudioPlayer::suspend()
{
  return [audioPlayer_ suspend];
}

float IOSAudioPlayer::getSampleRate() const
{
  return [audioPlayer_ getSampleRate];
}

} // namespace audioapi
