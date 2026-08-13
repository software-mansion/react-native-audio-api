#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace audioapi::decoding {

struct LocalFileSource {
  std::string path;
  int sampleRate = 0;
};

struct EncodedMemorySource {
  std::vector<uint8_t> data;
  int sampleRate = 0;
};

struct RemoteUrlSource {
  std::string url;
  std::map<std::string, std::string> httpHeaders;
  int sampleRate = 0;
};

struct RawPcmSource {
  std::vector<uint8_t> data;
  int channelCount = 0;
  bool interleaved = true;
  int sampleRate = 0;
};

struct RawPcmBase64Source {
  std::string base64;
  int channelCount = 0;
  bool interleaved = true;
  int sampleRate = 0;
};

using DecoderSource = std::variant<
    LocalFileSource,
    EncodedMemorySource,
    RemoteUrlSource,
    RawPcmSource,
    RawPcmBase64Source>;

[[nodiscard]] inline bool pathHasExtension(
    const std::string &path,
    const std::vector<std::string> &extensions) {
  std::string pathLower = path;
  std::ranges::transform(pathLower, pathLower.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return std::ranges::any_of(
      extensions, [&pathLower](const std::string &ext) { return pathLower.ends_with(ext); });
}

} // namespace audioapi::decoding
