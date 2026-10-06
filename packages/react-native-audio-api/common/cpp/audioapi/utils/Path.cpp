#include <audioapi/utils/Path.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace audioapi::path {

namespace {

constexpr const char *FILE_URL_PREFIX = "file://";
constexpr int HEX_BASE = 16;
constexpr size_t PERCENT_ESCAPE_HEX_DIGITS = 2;

} // namespace

std::string lowercaseExtension(const std::string &path) {
  std::string extension = std::filesystem::path(path).extension().string();
  if (extension.empty()) {
    return extension;
  }

  extension.erase(0, 1);
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  return extension;
}

bool hasExtension(const std::string &path, const std::vector<std::string> &extensions) {
  return std::ranges::find(extensions, lowercaseExtension(path)) != extensions.end();
}

bool hasNonFileProtocol(const std::string &path) {
  const auto colon = path.find(':');
  if (colon == std::string::npos) {
    return false;
  }

  const auto firstSlash = path.find('/');
  return firstSlash == std::string::npos || colon < firstSlash;
}

std::string percentDecode(const std::string &path) {
  std::string decoded;
  decoded.reserve(path.size());

  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i] != '%' || i + PERCENT_ESCAPE_HEX_DIGITS >= path.size()) {
      decoded.push_back(path[i]);
      continue;
    }

    unsigned int value = 0;
    const auto *first = path.data() + i + 1;
    const auto *last = first + PERCENT_ESCAPE_HEX_DIGITS;
    auto result = std::from_chars(first, last, value, HEX_BASE);
    if (result.ec != std::errc() || result.ptr != last) {
      decoded.push_back(path[i]);
      continue;
    }

    decoded.push_back(static_cast<char>(value));
    i += PERCENT_ESCAPE_HEX_DIGITS;
  }

  return decoded;
}

std::string normalizeFilePath(const std::string &path) {
  if (path.starts_with(FILE_URL_PREFIX)) {
    return percentDecode(path.substr(std::strlen(FILE_URL_PREFIX)));
  }

  return percentDecode(path);
}

} // namespace audioapi::path
