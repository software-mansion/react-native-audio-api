#pragma once

#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace recording_file_name {

/// Local-time stamp that keeps generated names apart between sessions.
std::string sessionTimestamp();

/// The stem every file of one recording session is built from. Resolved once per session
std::string sessionStem(const std::shared_ptr<AudioFileProperties> &properties);

/// The stem of one rotated segment. @p segmentIndex is 1-based and zero-padded
std::string segmentStem(const std::string &sessionStem, size_t segmentIndex);

/// Rejects a user-chosen file name that cannot produce a usable file. An empty name is valid:
/// the session then generates one.
Result<NoneType, std::string> validateFileName(const std::string &fileName);

} // namespace recording_file_name

} // namespace audioapi
