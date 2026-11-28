#pragma once

#include <string>
#include <vector>

namespace facebook::jsi {
class Runtime;
class Value;
} // namespace facebook::jsi

namespace audioapi {

/// @brief What a stopped recording session produced; mirrors the JS `FileInfo` returned by
/// `AudioRecorder.stop()`.
struct FileInfo {
  /// file:// URIs of every file the session wrote, in the order they were opened.
  std::vector<std::string> paths;
  /// Summed over all files, in megabytes.
  double size = 0.0;
  /// Summed over all files, in seconds.
  double duration = 0.0;

  [[nodiscard]] facebook::jsi::Value toJSIValue(facebook::jsi::Runtime &runtime) const;
};

/// @brief The one file `encodeAudioData` wrote; mirrors the JS `EncodedFileInfo`.
struct EncodedFileInfo {
  /// file:// URI of the file.
  std::string path;
  /// In megabytes.
  double size = 0.0;
  /// In seconds.
  double duration = 0.0;

  [[nodiscard]] facebook::jsi::Value toJSIValue(facebook::jsi::Runtime &runtime) const;
};

} // namespace audioapi
