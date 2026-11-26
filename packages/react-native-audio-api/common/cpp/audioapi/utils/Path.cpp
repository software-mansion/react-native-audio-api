#include <audioapi/utils/Path.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace audioapi::path {

namespace {

constexpr std::string_view FILE_URL_PREFIX = "file://";
constexpr int HEX_BASE = 16;
constexpr size_t PERCENT_ESCAPE_HEX_DIGITS = 2;

} // namespace

std::string lowercaseExtension(std::string_view path) {
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

bool hasExtension(std::string_view path, const std::vector<std::string> &extensions) {
  return std::ranges::find(extensions, lowercaseExtension(path)) != extensions.end();
}

bool hasNonFileProtocol(std::string_view path) {
  const auto colon = path.find(':');
  if (colon == std::string_view::npos) {
    return false;
  }

  const auto firstSlash = path.find('/');
  return firstSlash == std::string_view::npos || colon < firstSlash;
}

std::string percentDecode(std::string_view path) {
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

std::string normalizeFilePath(std::string_view path) {
  if (path.starts_with(FILE_URL_PREFIX)) {
    return percentDecode(path.substr(FILE_URL_PREFIX.size()));
  }

  return percentDecode(path);
}

std::string toFileUrl(std::string_view path) {
  constexpr std::string_view HEX_DIGITS = "0123456789ABCDEF";
  constexpr std::string_view UNESCAPED_PUNCTUATION = "/-._~";
  std::string url{FILE_URL_PREFIX};
  url.reserve(url.size() + path.size());

  for (const unsigned char c : path) {
    if (std::isalnum(c) != 0 ||
        UNESCAPED_PUNCTUATION.find(static_cast<char>(c)) != std::string_view::npos) {
      url.push_back(static_cast<char>(c));
      continue;
    }

    url.push_back('%');
    url.push_back(HEX_DIGITS[c / HEX_BASE]);
    url.push_back(HEX_DIGITS[c % HEX_BASE]);
  }

  return url;
}

} // namespace audioapi::path
