#include "AudioDecoder.h"
#include <fstream>
#include <iostream>
// #include <curl/curl.h>
#include <sndfile.h>
#include <chrono>
#include <filesystem>
#include "AudioArray.h"
#include "AudioBus.h"

#include <android/log.h>
#define LOG_TAG "AudioDecoder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace audioapi {

AudioDecoder::AudioDecoder(int sampleRate) : sampleRate_(sampleRate) {}

AudioBus *AudioDecoder::decode(const std::string &pathOrURL) {
  return decodeWithFilePath("/Files/runaway_kanye_west.mp3");
}

AudioBus *AudioDecoder::decodeWithFilePath(const std::string &path) const {
  SF_INFO sfInfo;
  SNDFILE *sndFile = sf_open(path.c_str(), SFM_READ, &sfInfo);
  if (!sndFile) {
    LOGE("Error opening audio file: %s", sf_strerror(sndFile));
    return nullptr;
  }

  auto *buffer = new float[sfInfo.frames * sfInfo.channels];
  sf_readf_float(sndFile, buffer, sfInfo.frames);
  sf_close(sndFile);

  if (sfInfo.samplerate != sampleRate_) {
    // todo
  }

  auto *audioBus = new AudioBus(
      sampleRate_, static_cast<int>(sfInfo.frames), sfInfo.channels);

  for (int i = 0; i < sfInfo.channels; ++i) {
    float *data = audioBus->getChannel(i)->getData();

    for (int j = 0; j < sfInfo.frames; ++j) {
      data[j] = buffer[j * sfInfo.channels + i];
    }
  }

  delete[] buffer;

  return audioBus;
}
} // namespace audioapi
