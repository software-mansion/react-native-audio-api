#pragma once

#include <string>
#include <vector>

/// String operations on local file paths and file:// URLs; nothing here touches the disk.
namespace audioapi::path {

/// Without the leading dot, e.g. "wav" for "/tmp/Take.WAV"; empty when there is none.
[[nodiscard]] std::string lowercaseExtension(const std::string &path);

/// @p extensions are lowercase and without the leading dot.
[[nodiscard]] bool hasExtension(
    const std::string &path,
    const std::vector<std::string> &extensions);

/// True for URLs such as http:// or content:// that do not name a local file.
[[nodiscard]] bool hasNonFileProtocol(const std::string &path);

/// Decodes %XX escapes; a malformed escape is kept verbatim.
[[nodiscard]] std::string percentDecode(const std::string &path);

/// Converts a local path or file:// URL into a decoded filesystem path.
[[nodiscard]] std::string normalizeFilePath(const std::string &path);

} // namespace audioapi::path
