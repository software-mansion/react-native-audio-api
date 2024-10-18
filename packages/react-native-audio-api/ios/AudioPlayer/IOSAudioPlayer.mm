#include <IOSAudioPlayer.h>

namespace audioapi {

IOSAudioPlayer::IOSAudioPlayer(const std::function<void(float *, int)> &renderAudio) : renderAudio_(renderAudio)
{
  RenderAudioBlock renderAudioBlock = ^(float *audioData, int numFrames) {
    renderAudio_(audioData, numFrames);
  };

  audioPlayer_ = [[AudioPlayer alloc] initWithRenderAudioBlock:renderAudioBlock];
}

IOSAudioPlayer::~IOSAudioPlayer()
{
  stop();
  [audioPlayer_ cleanup];
}

int IOSAudioPlayer::getSampleRate() const
{
  return [audioPlayer_ getSampleRate];
}

void IOSAudioPlayer::start()
{
  return [audioPlayer_ start];
}

void IOSAudioPlayer::stop()
{
  return [audioPlayer_ stop];
}
} // namespace audioapi
