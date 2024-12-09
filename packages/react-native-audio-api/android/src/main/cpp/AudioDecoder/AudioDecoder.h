#pragma once

#include <string>

namespace audioapi {

class AudioBus;

class AudioDecoder {
 public:
  explicit AudioDecoder(int sampleRate);

  AudioBus *decode(const std::string &pathOrURL);

 private:
  int sampleRate_;

  [[nodiscard]] AudioBus *decodeWithFilePath(const std::string &path) const;
  //        AudioBus *decodeWithURL(const std::string& url);

  //        void downloadFileFromURL(const std::string& url, const std::string&
  //        tempFilePath); static size_t WriteCallback(void* contents, size_t
  //        size, size_t nmemb, void* userp);
};

} // namespace audioapi
