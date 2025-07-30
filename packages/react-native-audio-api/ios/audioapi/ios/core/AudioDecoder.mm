#define MINIAUDIO_IMPLEMENTATION
#import <audioapi/libs/miniaudio/miniaudio.h>

#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>

#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/audio-stretch/stretch.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

std::shared_ptr<AudioBus> AudioDecoder::decodeWithFilePath(const std::string &path) const
{
  ma_decoding_backend_vtable *customBackends[] = {ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_s16, numChannels_, static_cast<int>(sampleRate_));
  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

  ma_result result = ma_decoder_init_file(path.c_str(), &config, &decoder);
  if (result != MA_SUCCESS) {
    NSLog(@"Failed to initialize decoder for file: %s", path.c_str());
    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  ma_uint64 totalFrameCount;
  ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrameCount);

  std::vector<int16_t> buffer(totalFrameCount * numChannels_);

  ma_uint64 framesDecoded;
  ma_decoder_read_pcm_frames(&decoder, buffer.data(), totalFrameCount, &framesDecoded);
  if (framesDecoded == 0) {
    NSLog(@"Failed to decode");
    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  auto outputFrames = buffer.size() / numChannels_;
  auto audioBus = std::make_shared<AudioBus>(outputFrames, numChannels_, sampleRate_);

  for (int i = 0; i < numChannels_; ++i) {
    auto channelData = audioBus->getChannel(i)->getData();
    for (int j = 0; j < outputFrames; ++j) {
      channelData[j] = int16ToFloat(buffer[j * numChannels_ + i]);
    }
  }

  ma_decoder_uninit(&decoder);

  return audioBus;
}

std::shared_ptr<AudioBus> AudioDecoder::decodeWithMemoryBlock(const void *data, size_t size) const
{
  ma_decoding_backend_vtable *customBackends[] = {ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_s16, numChannels_, static_cast<int>(sampleRate_));
  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

  ma_result result = ma_decoder_init_memory(data, size, &config, &decoder);
  if (result != MA_SUCCESS) {
    NSLog(@"Failed to initialize decoder for memory block");

    ma_decoder_uninit(&decoder);

    return nullptr;
  }

  ma_uint64 totalFrameCount;
  ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrameCount);

  std::vector<int16_t> buffer(totalFrameCount * numChannels_);

  ma_uint64 framesDecoded;
  ma_decoder_read_pcm_frames(&decoder, buffer.data(), totalFrameCount, &framesDecoded);

  if (framesDecoded == 0) {
    NSLog(@"Failed to decode");

    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  auto outputFrames = buffer.size() / numChannels_;
  auto audioBus = std::make_shared<AudioBus>(outputFrames, numChannels_, sampleRate_);

  for (int i = 0; i < numChannels_; ++i) {
    auto channelData = audioBus->getChannel(i)->getData();
    for (int j = 0; j < outputFrames; ++j) {
      channelData[j] = int16ToFloat(buffer[j * numChannels_ + i]);
    }
  }

  ma_decoder_uninit(&decoder);

  return audioBus;
}

std::shared_ptr<AudioBus> AudioDecoder::decodeWithPCMInBase64(const std::string &data, float playbackSpeed) const
{
  auto decodedData = base64_decode(data, false);

  const auto uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t framesDecoded = decodedData.size() / 2;

  std::vector<int16_t> buffer(framesDecoded);
  for (size_t i = 0; i < framesDecoded; ++i) {
    buffer[i] = static_cast<int16_t>((uint8Data[i * 2 + 1] << 8) | uint8Data[i * 2]);
  }

  changePlaybackSpeedIfNeeded(buffer, framesDecoded, 1, playbackSpeed);
  auto outputFrames = buffer.size();

  auto audioBus = std::make_shared<AudioBus>(outputFrames, numChannels_, sampleRate_);
  auto leftChannelData = audioBus->getChannel(0)->getData();
  auto rightChannelData = audioBus->getChannel(1)->getData();

  for (size_t i = 0; i < outputFrames; ++i) {
    auto sample = int16ToFloat(buffer[i]);
    leftChannelData[i] = sample;
    rightChannelData[i] = sample;
  }

  return audioBus;
}

} // namespace audioapi
