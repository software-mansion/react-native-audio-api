#include "AudioDecoder.h"

#define MINIAUDIO_IMPLEMENTATION
#include <android/log.h>
#include "miniaudio.h"

#include "AudioArray.h"
#include "AudioBus.h"

namespace audioapi {

AudioDecoder::AudioDecoder(int sampleRate) : sampleRate_(sampleRate) {}

AudioBus *AudioDecoder::decodeWithFilePath(const std::string &path) const {
  ma_decoder decoder;
  ma_decoder_config config =
      ma_decoder_config_init(ma_format_f32, 2, sampleRate_);
  ma_result result = ma_decoder_init_file(path.c_str(), &config, &decoder);
  if (result != MA_SUCCESS) {
    __android_log_print(
        ANDROID_LOG_ERROR,
        "AudioDecoder",
        "Failed to initialize decoder for file: %s",
        path.c_str());
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
    __android_log_print(
        ANDROID_LOG_ERROR,
        "AudioDecoder",
        "Failed to decode audio file: %s",
        path.c_str());
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

AudioBus *AudioDecoder::decodeWithArrayBuffer() const {
  // TODO: implement this
  return new AudioBus(1, 1, 1);
}

} // namespace audioapi
