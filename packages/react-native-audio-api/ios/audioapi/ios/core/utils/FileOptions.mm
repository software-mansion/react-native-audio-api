#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/utils/AudioFileProperties.h>

namespace audioapi::ios::fileoptions {

/// @brief Maps AudioFileProperties to iOS AVFoundation audio quality settings.
/// @param properties Shared pointer to AudioFileProperties.
/// @returns Corresponding NSInteger value for AVAudioQuality.
NSInteger getQuality(const std::shared_ptr<AudioFileProperties> &properties)
{
  switch (properties->iosAudioQuality) {
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

/// @brief Retrieves the FLAC compression level from AudioFileProperties.
/// @param properties Shared pointer to AudioFileProperties.
/// @returns NSInteger representing the FLAC compression level.
NSInteger getFlacCompressionLevel(const std::shared_ptr<AudioFileProperties> &properties)
{
  return properties->flacCompressionLevel;
}

/// @brief Retrieves the bit depth from AudioFileProperties.
/// @param properties Shared pointer to AudioFileProperties.
/// @returns NSInteger representing the bit depth.
NSInteger getBitDepth(const std::shared_ptr<AudioFileProperties> &properties)
{
  switch (properties->bitDepth) {
    case AudioFileProperties::BitDepth::Bit16:
      return 16;

    case AudioFileProperties::BitDepth::Bit24:
      return 24;

    case AudioFileProperties::BitDepth::Bit32:
    default:
      return 32;
  }
}

} // namespace audioapi::ios::fileoptions
