#pragma once

#include <audioapi/utils/Result.hpp>

#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <audioapi/android/AndroidRemux.h>
#define RN_AUDIO_API_HAS_OS_REMUX 1
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST)
#include <audioapi/ios/core/utils/IOSRemux.h>
#define RN_AUDIO_API_HAS_OS_REMUX 1
#else
#define RN_AUDIO_API_HAS_OS_REMUX 0
#endif

namespace audioapi {

using AudioRemuxResult = Result<std::string, std::string>;

inline AudioRemuxResult remuxConcatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
#if defined(__ANDROID__)
  return android_remux::concatAudioFiles(inputPaths, outputPath);
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST)
  return ios_remux::concatAudioFiles(inputPaths, outputPath);
#else
  (void)inputPaths;
  (void)outputPath;
  return Err("concatAudioFiles remux requires iOS or Android.");
#endif
}

} // namespace audioapi
