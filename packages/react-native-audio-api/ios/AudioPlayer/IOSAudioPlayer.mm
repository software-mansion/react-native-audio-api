#include <IOSAudioPlayer.h>
#include "AudioContext.h"

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(AudioContext *context): context_(context)
{
  audioPlayer_ = [[AudioPlayer alloc] init];
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  //cleanup
  audioPlayer_ = nil;
}

int IOSAudioPlayer::getSampleRate() const
{
  return [audioPlayer_ getSampleRate];
}

int IOSAudioPlayer::getFramesPerBurst() const
{
  return [audioPlayer_ getFramesPerBurst];
}

void IOSAudioPlayer::start()
{
  return [audioPlayer_ start];
}

void IOSAudioPlayer::stop()
{
  return [audioPlayer_ stop];
}

void IOSAudioPlayer::renderAudio(float *audioData, int32_t numFrames)
{
  context_->getSampleRate();
}
} // namespace audioapi

