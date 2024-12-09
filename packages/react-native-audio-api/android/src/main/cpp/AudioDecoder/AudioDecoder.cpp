#include "AudioDecoder.h"
#include <fstream>
#include <iostream>
// #include <curl/curl.h>
#include <sndfile.h>
#include <samplerate.h>
#include <chrono>
#include <filesystem>

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
    // mp3 unsupported
  SF_INFO sfInfo;
  SNDFILE *sndFile = sf_open(path.c_str(), SFM_READ, &sfInfo);

  if (!sndFile) {
    LOGE("Error opening audio file: %s", sf_strerror(sndFile));
    return new AudioBus(1, 1, 1);
  }

  auto *buffer = new float[sfInfo.frames * sfInfo.channels];
  sf_readf_float(sndFile, buffer, sfInfo.frames * sfInfo.channels);

    float *outputBuffer = nullptr;
    size_t outputFrames = sfInfo.frames;
    int outputChannels = sfInfo.channels;
  if (sfInfo.samplerate != sampleRate_) {
      float src_ratio = static_cast<float>(sampleRate_) / sfInfo.samplerate;
      outputFrames = lround(static_cast<float>(outputFrames) * src_ratio + .5);

      outputBuffer = new float[outputFrames * outputChannels];
      SRC_DATA src_data;
      src_data.data_in = buffer;
      src_data.input_frames = static_cast<long>(sfInfo.frames);
      src_data.data_out = outputBuffer;
      src_data.output_frames = static_cast<long>(outputFrames);
      src_data.src_ratio = src_ratio;
      src_data.end_of_input = 1;

      int src_error = src_simple(&src_data, SRC_SINC_FASTEST, sfInfo.channels);
      if (src_error) {
          LOGE("Sample rate conversion error: %s", src_strerror(src_error));
      }
  } else {
      outputBuffer = buffer;
  }

  auto *audioBus = new AudioBus(
      sampleRate_, static_cast<int>(outputFrames), outputChannels);

  for (int i = 0; i < outputChannels; ++i) {
    float *data = audioBus->getChannel(i)->getData();

    for (int j = 0; j < outputFrames; ++j) {
      data[j] = outputBuffer[j * sfInfo.channels + i];
    }
  }

  sf_close(sndFile);
  delete[] buffer;

  return audioBus;
}

} // namespace audioapi
