#include <IOSAudioPlayere.h>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer()
{
  audioPlayer_ = nil;
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  // audioPlayer_ stop
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
  //todo
}
} // namespace audioapi

