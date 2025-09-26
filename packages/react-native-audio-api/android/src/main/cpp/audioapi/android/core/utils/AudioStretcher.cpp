#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioStretcher.h>
#include <audioapi/libs/audio-stretch/stretch.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <cstdint>

namespace audioapi {

std::vector<int16_t> AudioStretcher::castToInt16Buffer(
    const float *data,
    size_t size) const {
  const size_t numChannels = 2;
  std::vector<int16_t> int16Buffer(size);

  for (size_t i = 0; i < size; ++i) {
    int16Buffer[i] = floatToInt16(data[i]);
  }
  return int16Buffer;
}

std::shared_ptr<AudioBuffer> AudioStretcher::changePlaybackSpeed(
    AudioBuffer buffer,
    float playbackSpeed) const {
  // TODO: handle multiple channels
  const size_t numChannels = 2;
  const size_t numFrames = buffer.getLength() / numChannels;

  // if (playbackSpeed == 1.0f) {
  //   auto audioBus =
  //       std::make_shared<AudioBus>(numFrames, numChannels, sampleRate_);
  //   auto leftChannelData = audioBus->getChannel(0)->getData();
  //   auto rightChannelData = audioBus->getChannel(1)->getData();

  //   for (size_t i = 0; i < numFrames; ++i) {
  //     float sample = data[i];
  //     leftChannelData[i] = sample;
  //     rightChannelData[i] = sample;
  //   }

  //   return std::make_shared<AudioBuffer>(audioBus);
  // }

  std::vector<int16_t> int16Buffer =
      castToInt16Buffer(buffer.getChannelData(0), buffer.getLength());

  auto stretcher = stretch_init(
      static_cast<int>(sampleRate_ / 333.0f),
      static_cast<int>(sampleRate_ / 55.0f),
      numChannels,
      0x1);

  int maxOutputFrames = stretch_output_capacity(
      stretcher, static_cast<int>(numFrames), 1 / playbackSpeed);
  std::vector<int16_t> stretchedBuffer(maxOutputFrames * numChannels);

  int outputFrames = stretch_samples(
      stretcher,
      int16Buffer.data(),
      static_cast<int>(numFrames),
      stretchedBuffer.data(),
      1 / playbackSpeed);

  outputFrames +=
      stretch_flush(stretcher, stretchedBuffer.data() + (outputFrames));
  stretchedBuffer.resize(outputFrames);
  stretch_deinit(stretcher);

  auto audioBus =
      std::make_shared<AudioBus>(outputFrames, numChannels, sampleRate_);
  auto leftChannelData = audioBus->getChannel(0)->getData();
  auto rightChannelData = audioBus->getChannel(1)->getData();

  for (size_t i = 0; i < outputFrames; ++i) {
    float sample = int16ToFloat(stretchedBuffer[i]);
    leftChannelData[i] = sample;
    rightChannelData[i] = sample;
  }

  return std::make_shared<AudioBuffer>(audioBus);
}

} // namespace audioapi
