#pragma once

#if defined(__ANDROID__)
#include <audioapi/android/AndroidEncoder.h>
#define RN_AUDIO_API_HAS_OS_ENCODER 1
namespace audioapi::os_encoder {
using Encoder = android::encoder::AndroidEncoder;
} // namespace audioapi::os_encoder
#elif defined(__APPLE__) && !defined(RN_AUDIO_API_TEST) && !defined(RN_AUDIO_API_NODE)
#include <audioapi/ios/core/utils/IOSEncoder.h>
#define RN_AUDIO_API_HAS_OS_ENCODER 1
namespace audioapi::os_encoder {
using Encoder = ios::encoder::IOSEncoder;
} // namespace audioapi::os_encoder
#else
#define RN_AUDIO_API_HAS_OS_ENCODER 0
#endif
