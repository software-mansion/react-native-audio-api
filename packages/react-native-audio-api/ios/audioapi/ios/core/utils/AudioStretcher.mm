#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioStretcher.h>

namespace audioapi {

void AudioStretcher::changePlaybackSpeed(AudioBuffer &audioBuffer, float playbackSpeed) const
{
  if (playbackSpeed == 1.0f) {
    return;
  }

  // TODO: handle multiple channels
  const size_t numChannels = 2;
  const size_t numFrames = audioBuffer.getLength();

  std::vector<int16_t> int16Buffer(numFrames);

  auto channelData = audioBuffer.getChannelData(0);
  for (int i = 0; i < numFrames; ++i) {
    int16Buffer[i] = floatToInt16(channelData[i]);
  }

  auto stretcher =
      stretch_init(static_cast<int>(sampleRate_ / 333.0f), static_cast<int>(sampleRate_ / 55.0f), numChannels, 0x1);

  int maxOutputFrames = stretch_output_capacity(stretcher, static_cast<int>(numFrames), 1 / playbackSpeed);
  std::vector<int16_t> stretchedBuffer(maxOutputFrames);

  int outputFrames = stretch_samples(
      stretcher, int16Buffer.data(), static_cast<int>(numFrames), stretchedBuffer.data(), 1 / playbackSpeed);

  outputFrames += stretch_flush(stretcher, stretchedBuffer.data() + (outputFrames));
  stretchedBuffer.resize(outputFrames);

  auto leftChannelData = audioBuffer.getChannelData(0);
  auto rightChannelData = audioBuffer.getChannelData(1);

  leftChannelData.resize(outputFrames);
  rightChannelData.resize(outputFrames);

  for (size_t i = 0; i < outputFrames; ++i) {
    float sample = int16ToFloat(stretchedBuffer[i]);
    leftChannelData[i] = sample;
    rightChannelData[i] = sample;
  }

  stretch_deinit(stretcher);
}

} // namespace audioapi
