#include "AudioDecoder.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioArray.h"
#include "AudioBus.h"

#include <android/log.h>
#define LOG_TAG "AudioDecoder"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace audioapi {

AudioDecoder::AudioDecoder(int sampleRate) : sampleRate_(sampleRate) {}

AudioBus *AudioDecoder::decode(const std::string &pathOrURL) {
  return decodeWithFilePath(pathOrURL);
}

AudioBus *AudioDecoder::decodeWithFilePath(const std::string &path) const {
  ma_decoder decoder;
  ma_decoder_config config =
      ma_decoder_config_init(ma_format_f32, 2, sampleRate_);
  ma_result result = ma_decoder_init_file(path.c_str(), &config, &decoder);
  if (result != MA_SUCCESS) {
    LOGE("Failed to initialize decoder for file: %s", path.c_str());
    return new AudioBus(1, 1, 1);
  }

  ma_uint64 totalFrameCount;
  ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrameCount);

  auto *audioBus =
      new AudioBus(sampleRate_, static_cast<int>(totalFrameCount), 2);
  auto *buffer = new float[totalFrameCount * 2];

  ma_uint64 framesDecoded;
  ma_decoder_read_pcm_frames(&decoder, buffer, totalFrameCount, &framesDecoded);
  if (framesDecoded == 0) {
    LOGE("Failed to read any frames from decoder");
  }

  for (int i = 0; i < decoder.outputChannels; ++i) {
    float *channelData = audioBus->getChannel(i)->getData();

    for (ma_uint64 j = 0; j < framesDecoded; ++j) {
      channelData[j] = buffer[j * decoder.outputChannels + i];
    }
  }

  delete[] buffer;
  ma_decoder_uninit(&decoder);

  return audioBus;
}

} // namespace audioapi
