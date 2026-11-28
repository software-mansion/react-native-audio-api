#pragma once

#include <string>
#include <string_view>
#include <vector>

/// String operations on local file paths and file:// URLs; nothing here touches the disk.
namespace audioapi::path {

/// Without the leading dot, e.g. "wav" for "/tmp/Take.WAV"; empty when there is none.
[[nodiscard]] std::string lowercaseExtension(std::string_view path);

/// @p extensions are lowercase and without the leading dot.
[[nodiscard]] bool hasExtension(std::string_view path, const std::vector<std::string> &extensions);

/// True for URLs such as http:// or content:// that do not name a local file.
[[nodiscard]] bool hasNonFileProtocol(std::string_view path);

/// Decodes %XX escapes; a malformed escape is kept verbatim.
[[nodiscard]] std::string percentDecode(std::string_view path);

/// Converts a local path or file:// URL into a decoded filesystem path.
[[nodiscard]] std::string normalizeFilePath(std::string_view path);

/// The file:// URL JS receives for the filesystem @p path; everything but unreserved characters
/// and '/' is percent-encoded, so normalizeFilePath() maps it back to @p path.
[[nodiscard]] std::string toFileUrl(std::string_view path);

} // namespace audioapi::path
