#pragma once

/**
 * Selects the OS-native incremental decoder for the current mobile target.
 *
 *   - Android → AndroidDecoder
 *   - iOS     → IOSDecoder
 *
 * Not available on desktop builds — the C++ unit tests (RN_AUDIO_API_TEST) and the
 * Node.js addon used by the WPT harness (RN_AUDIO_API_NODE) run on macOS but cannot
 * link against the iOS-only sources.
 */

#if defined(__ANDROID__)
#include <audioapi/android/AndroidDecoding.h>
#define RN_AUDIO_API_HAS_OS_DECODER 1
namespace audioapi::os_decoder {
using Decoder = android_decoder::AndroidDecoder;
} // namespace audioapi::os_decoder
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST) && !defined(RN_AUDIO_API_NODE)
#include <audioapi/ios/core/utils/IOSDecoding.h>
#define RN_AUDIO_API_HAS_OS_DECODER 1
namespace audioapi::os_decoder {
using Decoder = ios_decoder::IOSDecoder;
} // namespace audioapi::os_decoder
#else
#define RN_AUDIO_API_HAS_OS_DECODER 0
#endif
