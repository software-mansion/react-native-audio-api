#pragma once

#include <memory>
#include <string>
#include <vector>
// #include <curl/curl.h>
#include <sndfile.h>
#include <filesystem>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(int sampleRate);

  AudioBus *decode(const std::string &pathOrURL);
  //float *convertBuffer(const float *buffer);

 private:
  int sampleRate_;

  AudioBus *decodeWithFilePath(const std::string &path) const;
  //        AudioBus *decodeWithURL(const std::string& url);

  //        void downloadFileFromURL(const std::string& url, const std::string&
  //        tempFilePath); static size_t WriteCallback(void* contents, size_t
  //        size, size_t nmemb, void* userp);
};

} // namespace audioapi
