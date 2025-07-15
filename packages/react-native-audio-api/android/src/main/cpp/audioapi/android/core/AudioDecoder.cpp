#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#define MINIAUDIO_IMPLEMENTATION
#include <audioapi/libs/miniaudio/miniaudio.h>

// #include <android/log.h>

namespace audioapi {

std::shared_ptr<AudioBus> AudioDecoder::decodeWithFilePath(
    const std::string &path) const {
  ma_decoder decoder;
  ma_decoder_config config =
      ma_decoder_config_init(ma_format_f32, 2, static_cast<int>(sampleRate_));
  ma_result result = ma_decoder_init_file(path.c_str(), &config, &decoder);
  if (result != MA_SUCCESS) {
    // __android_log_print(
    //   ANDROID_LOG_ERROR,
    //   "AudioDecoder",
    //   "Failed to initialize decoder for file: %s",
    //   path.c_str());
    ma_decoder_uninit(&decoder);

    return nullptr;
  }

  ma_uint64 totalFrameCount;
  ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrameCount);

  auto audioBus = std::make_shared<AudioBus>(
      static_cast<int>(totalFrameCount), 2, sampleRate_);
  auto *buffer = new float[totalFrameCount * 2];

  ma_uint64 framesDecoded;
  ma_decoder_read_pcm_frames(&decoder, buffer, totalFrameCount, &framesDecoded);

  if (framesDecoded == 0) {
    // __android_log_print(ANDROID_LOG_ERROR, "AudioDecoder", "Failed to decode");

    delete[] buffer;
    ma_decoder_uninit(&decoder);

    return nullptr;
  }

  for (int i = 0; i < decoder.outputChannels; ++i) {
    auto channelData = audioBus->getChannel(i)->getData();

    for (ma_uint64 j = 0; j < framesDecoded; ++j) {
      channelData[j] = buffer[j * decoder.outputChannels + i];
    }
  }

  delete[] buffer;
  ma_decoder_uninit(&decoder);

  return audioBus;
}

std::shared_ptr<AudioBus> AudioDecoder::decodeWithMemoryBlock(
    const void *data,
    size_t size) const {
  ma_decoder decoder;
  ma_decoder_config config =
      ma_decoder_config_init(ma_format_f32, 2, static_cast<int>(sampleRate_));
  ma_result result = ma_decoder_init_memory(data, size, &config, &decoder);
  if (result != MA_SUCCESS) {
    // __android_log_print(
    //   ANDROID_LOG_ERROR,
    //   "AudioDecoder",
    //   "Failed to initialize decoder for memory block");
    ma_decoder_uninit(&decoder);

    return nullptr;
  }

  ma_uint64 totalFrameCount;
  ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrameCount);

  auto audioBus = std::make_shared<AudioBus>(
      static_cast<int>(totalFrameCount), 2, sampleRate_);
  auto *buffer = new float[totalFrameCount * 2];

  ma_uint64 framesDecoded;
  ma_decoder_read_pcm_frames(&decoder, buffer, totalFrameCount, &framesDecoded);
  if (framesDecoded == 0) {
    // __android_log_print(ANDROID_LOG_ERROR, "AudioDecoder", "Failed to decode");

    delete[] buffer;
    ma_decoder_uninit(&decoder);

    return nullptr;
  }

  for (int i = 0; i < decoder.outputChannels; ++i) {
    auto channelData = audioBus->getChannel(i)->getData();

    for (ma_uint64 j = 0; j < framesDecoded; ++j) {
      channelData[j] = buffer[j * decoder.outputChannels + i];
    }
  }

  delete[] buffer;
  ma_decoder_uninit(&decoder);

  return audioBus;
}

std::shared_ptr<AudioBus> AudioDecoder::decodeWithPCMInBase64(
    const std::string &data) const {
  auto decodedData = base64_decode(data, false);

  const auto uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t frameCount = decodedData.size() / 2;

  auto audioBus = std::make_shared<AudioBus>(frameCount, 1, sampleRate_);
  auto channelData = audioBus->getChannel(0)->getData();

  for (size_t i = 0; i < frameCount; ++i) {
    auto sample =
        static_cast<int16_t>((uint8Data[i * 2 + 1] << 8) | uint8Data[i * 2]);
    channelData[i] = static_cast<float>(sample);
  }

  dsp::multiplyByScalar(channelData, 1.0f / 32768.0f, channelData, frameCount);

  return audioBus;
}

} // namespace audioapi
