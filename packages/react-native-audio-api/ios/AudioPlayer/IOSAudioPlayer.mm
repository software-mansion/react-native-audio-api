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
    renderAudio_(audioBus_, numFrames);

    for (int i = 0; i < outputData->mNumberBuffers; i += 1) {
      float *outputBuffer = (float *)outputData->mBuffers[i].mData;

      memcpy(outputBuffer, audioBus_->getChannel(i)->getData(), sizeof(float) * numFrames);
    }
  };

  audioPlayer_ = [[AudioPlayer alloc] initWithRenderAudioBlock:renderAudioBlock];
  audioBus_ = new AudioBus(getSampleRate(), getBufferSizeInFrames(), CHANNEL_COUNT);
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

int IOSAudioPlayer::getBufferSizeInFrames() const
{
  return [audioPlayer_ getBufferSizeInFrames];
}

} // namespace audioapi
