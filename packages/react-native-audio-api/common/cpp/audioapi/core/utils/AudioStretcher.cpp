#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioStretcher.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/libs/audio-stretch/stretch.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace audioapi {

std::vector<int16_t> AudioStretcher::castToInt16Buffer(AudioBuffer &buffer) {
  const size_t numChannels = buffer.getNumberOfChannels();
  const size_t numFrames = buffer.getLength();

  std::vector<int16_t> int16Buffer(numFrames * numChannels);

  for (size_t ch = 0; ch < numChannels; ++ch) {
    const float *channelData = buffer.getChannelData(ch);
    for (size_t i = 0; i < numFrames; ++i) {
      int16Buffer[i * numChannels + ch] = floatToInt16(channelData[i]);
    }
  }

  return int16Buffer;
}

std::shared_ptr<AudioBuffer> AudioStretcher::changePlaybackSpeed(
    AudioBuffer buffer,
    float playbackSpeed) {
  const float sampleRate = buffer.getSampleRate();
  const size_t outputChannels = buffer.getNumberOfChannels();
  const size_t numFrames = buffer.getLength();

  if (playbackSpeed == 1.0f) {
    return std::make_shared<AudioBuffer>(buffer);
  }

  std::vector<int16_t> int16Buffer = castToInt16Buffer(buffer);

  auto stretcher = stretch_init(
      static_cast<int>(sampleRate / UPPER_FREQUENCY_LIMIT_DETECTION),
      static_cast<int>(sampleRate / LOWER_FREQUENCY_LIMIT_DETECTION),
      outputChannels,
      0x1);

  int maxOutputFrames =
      stretch_output_capacity(stretcher, static_cast<int>(numFrames), 1 / playbackSpeed);
  std::vector<int16_t> stretchedBuffer(maxOutputFrames * outputChannels);

  int outputFrames = stretch_samples(
      stretcher,
      int16Buffer.data(),
      static_cast<int>(numFrames),
      stretchedBuffer.data(),
      1 / playbackSpeed);

  outputFrames += stretch_flush(stretcher, stretchedBuffer.data() + (outputFrames));
  stretchedBuffer.resize(outputFrames * outputChannels);
  stretch_deinit(stretcher);

  auto audioBus = std::make_shared<AudioBus>(outputFrames, outputChannels, sampleRate);

  for (int ch = 0; ch < outputChannels; ++ch) {
    auto channelData = audioBus->getChannel(ch)->getData();
    for (int i = 0; i < outputFrames; ++i) {
      channelData[i] = int16ToFloat(stretchedBuffer[i * outputChannels + ch]);
    }
  }

  return std::make_shared<AudioBuffer>(audioBus);
}

} // namespace audioapi
