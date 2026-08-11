#pragma once

/**
 * Selects the OS-native incremental decoder for the current mobile target.
 *
 *   - Android → AndroidDecoder
 *   - iOS     → IOSDecoder
 *
 * Not available on desktop / unit-test builds (RN_AUDIO_API_TEST).
 */

#if defined(__ANDROID__)
#include <audioapi/android/AndroidDecoding.h>
#define RN_AUDIO_API_HAS_OS_DECODER 1
namespace audioapi::os_decoder {
using Decoder = android_decoder::AndroidDecoder;
} // namespace audioapi::os_decoder
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST)
#include <audioapi/ios/core/utils/IOSDecoding.h>
#define RN_AUDIO_API_HAS_OS_DECODER 1
namespace audioapi::os_decoder {
using Decoder = ios_decoder::IOSDecoder;
} // namespace audioapi::os_decoder
#else
#define RN_AUDIO_API_HAS_OS_DECODER 0
#endif
