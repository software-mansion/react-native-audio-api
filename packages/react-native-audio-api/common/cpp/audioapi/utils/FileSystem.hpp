#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <system_error>

namespace audioapi::file_system {

/// False as well when the path cannot be inspected.
[[nodiscard]] inline bool fileExists(const std::string &path) {
  std::error_code error;
  return std::filesystem::exists(path, error);
}

/// Zero when the file is missing or its size cannot be read.
[[nodiscard]] inline size_t fileSizeBytes(const std::string &path) {
  std::error_code error;
  const auto size = std::filesystem::file_size(path, error);
  return error ? 0 : static_cast<size_t>(size);
}

/// A missing file is not an error; neither is one that cannot be deleted.
inline void removeFile(const std::string &path) {
  std::error_code ignored;
  std::filesystem::remove(path, ignored);
}

/// Replaces @p toPath when it already exists. Returns the OS error when the move fails.
[[nodiscard]] inline std::error_code moveFile(
    const std::string &fromPath,
    const std::string &toPath) {
  std::error_code error;
  std::filesystem::rename(fromPath, toPath, error);
  return error;
}

} // namespace audioapi::file_system
