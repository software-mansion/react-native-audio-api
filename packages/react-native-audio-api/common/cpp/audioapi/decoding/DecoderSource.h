#pragma once

#include <audioapi/utils/AlignedAllocator.hpp>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace audioapi::decoding {

/// Bytes of a whole encoded file or raw PCM block. A decode holds them only until it
/// finishes, and `AlignedAllocator` returns a large block to the system as soon as it is freed.
using EncodedBytes = std::vector<uint8_t, AlignedAllocator<uint8_t>>;

struct LocalFileSource {
  std::string path;
  int sampleRate = 0;
};

struct EncodedMemorySource {
  EncodedBytes data;
  int sampleRate = 0;
};

struct RemoteUrlSource {
  std::string url;
  std::map<std::string, std::string> httpHeaders;
  int sampleRate = 0;
};

struct RawPcmSource {
  EncodedBytes data;
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
