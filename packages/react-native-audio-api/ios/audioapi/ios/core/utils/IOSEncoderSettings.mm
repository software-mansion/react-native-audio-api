#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/IOSEncoderSettings.h>
#include <audioapi/utils/AudioFileProperties.h>

namespace audioapi::ios_encoder {

/// @brief Maps the encoding settings to an iOS AVFoundation audio quality.
/// @param encoding The file's encoding settings.
/// @returns Corresponding NSInteger value for AVAudioQuality.
NSInteger getQuality(const AudioFileProperties::EncodingConfig &encoding)
{
  switch (encoding.iosAudioQuality) {
    case AudioFileProperties::IOSAudioQuality::Min:
      return AVAudioQualityMin;

    case AudioFileProperties::IOSAudioQuality::Low:
      return AVAudioQualityLow;

    case AudioFileProperties::IOSAudioQuality::Medium:
      return AVAudioQualityMedium;

    case AudioFileProperties::IOSAudioQuality::High:
      return AVAudioQualityHigh;

    case AudioFileProperties::IOSAudioQuality::Max:
      return AVAudioQualityMax;

    default:
      return AVAudioQualityMedium;
  }
}

/// @brief Retrieves the FLAC compression level from the encoding settings.
/// @param encoding The file's encoding settings.
/// @returns NSInteger representing the FLAC compression level.
NSInteger getFlacCompressionLevel(const AudioFileProperties::EncodingConfig &encoding)
{
  return encoding.flacCompressionLevel;
}

/// @brief Retrieves the bit depth from the encoding settings.
/// @param encoding The file's encoding settings.
/// @returns NSInteger representing the bit depth.
NSInteger getBitDepth(const AudioFileProperties::EncodingConfig &encoding)
{
  switch (encoding.bitDepth) {
    case AudioFileProperties::BitDepth::Bit16:
      return 16;

    case AudioFileProperties::BitDepth::Bit24:
      return 24;

    case AudioFileProperties::BitDepth::Bit32:
    default:
      return 32;
  }
}

} // namespace audioapi::ios_encoder
