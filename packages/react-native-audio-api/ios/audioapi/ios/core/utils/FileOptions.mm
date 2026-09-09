#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <string>

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

NSURL *getFileURL(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName)
{
  NSError *error = nil;

  NSSearchPathDirectory directory = getDirectory(properties);
  NSString *subDirectory = [NSString stringWithUTF8String:properties->subDirectory.c_str()];

  NSURL *baseURL = [[[NSFileManager defaultManager] URLsForDirectory:directory
                                                           inDomains:NSUserDomainMask] firstObject];
  NSURL *directoryURL = [baseURL URLByAppendingPathComponent:subDirectory isDirectory:YES];

  [[NSFileManager defaultManager] createDirectoryAtURL:directoryURL
                           withIntermediateDirectories:YES
                                            attributes:nil
                                                 error:&error];

  if (error != nil) {
    NSLog(@"Error creating directory for audio recordings: %@", [error debugDescription]);
    directoryURL = baseURL;
  }

  return
      [directoryURL URLByAppendingPathComponent:[NSString stringWithUTF8String:fileName.c_str()]];
}

NSSearchPathDirectory getDirectory(const std::shared_ptr<AudioFileProperties> &properties)
{
  switch (properties->directory) {
    case AudioFileProperties::FileDirectory::Document:
      return NSDocumentDirectory;

    case AudioFileProperties::FileDirectory::Cache:
    default:
      return NSCachesDirectory;
  }
}

} // namespace audioapi::ios::fileoptions
