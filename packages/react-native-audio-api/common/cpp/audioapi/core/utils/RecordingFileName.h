#pragma once

#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;

namespace recordingfilename {

/// Local-time stamp that keeps generated names apart between sessions.
std::string sessionTimestamp();

/// The stem every file of one recording session is built from. Resolved once per session
std::string sessionStem(const std::shared_ptr<AudioFileProperties> &properties);

/// The stem of one rotated segment. @p segmentIndex is 1-based and zero-padded
std::string segmentStem(const std::string &sessionStem, size_t segmentIndex);

/// Rejects a `fileName` that cannot produce a usable file.
/// @returns Ok when the properties are usable, Err describing what is wrong otherwise.
Result<NoneType, std::string> validate(const std::shared_ptr<AudioFileProperties> &properties);

} // namespace recordingfilename

} // namespace audioapi
