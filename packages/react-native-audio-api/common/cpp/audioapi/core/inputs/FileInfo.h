#pragma once

#include <string>
#include <vector>

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
};

} // namespace audioapi
