#pragma once

#include <audioapi/utils/Result.hpp>

#include <memory>
#include <string>

#if defined(__ANDROID__)
#include <audioapi/android/core/utils/FileOptions.h>
#define RN_AUDIO_API_HAS_OS_FILE_PATH 1
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST) && !defined(RN_AUDIO_API_NODE)
#include <audioapi/ios/core/utils/IOSFilePath.h>
#define RN_AUDIO_API_HAS_OS_FILE_PATH 1
#else
#define RN_AUDIO_API_HAS_OS_FILE_PATH 0
#endif

namespace audioapi {

class AudioFileProperties;

using ResolveFilePathResult = Result<std::string, std::string>;

/// Resolves the absolute output path for a recording and creates its directory.
inline ResolveFilePathResult resolveOsFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride) {
#if defined(__ANDROID__)
  return android::fileoptions::getFilePath(properties, fileNameOverride);
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST) && !defined(RN_AUDIO_API_NODE)
  return ios_filepath::resolveFilePath(properties, fileNameOverride);
#else
  (void)properties;
  (void)fileNameOverride;
  return ResolveFilePathResult::Err("File path resolution requires iOS or Android.");
#endif
}

} // namespace audioapi
