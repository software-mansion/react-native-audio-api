#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/utils/AudioFileProperties.h>

namespace audioapi::ios::fileoptions {

/// @brief Maps AudioFileProperties to iOS AVFoundation AudioFormatID.
/// @param properties Shared pointer to AudioFileProperties.
/// @returns Corresponding AudioFormatID for AVFoundation.
AudioFormatID getFormat(const std::shared_ptr<AudioFileProperties> &properties)
{
  switch (properties->format) {
    case AudioFileProperties::Format::WAV:
      return kAudioFormatLinearPCM;

    case AudioFileProperties::Format::CAF:
      return kAudioFormatLinearPCM;

    case AudioFileProperties::Format::M4A:
      return kAudioFormatMPEG4AAC;

    case AudioFileProperties::Format::FLAC:
      return kAudioFormatFLAC;

    default:
      return kAudioFormatLinearPCM;
  }
}

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

/// @brief Retrieves the file extension based on AudioFileProperties format.
/// @param properties Shared pointer to AudioFileProperties.
/// @returns NSString representing the file extension.
NSString *getFileExtension(const std::shared_ptr<AudioFileProperties> &properties)
{
  switch (properties->format) {
    case AudioFileProperties::Format::WAV:
      return @"wav";

    case AudioFileProperties::Format::CAF:
      return @"caf";

    case AudioFileProperties::Format::M4A:
      return @"m4a";

    case AudioFileProperties::Format::FLAC:
      return @"flac";

    default:
      return @"wav";
  }
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
      return 32;

    default:
      return 32;
  }
}

/// @brief Constructs AVFoundation file settings dictionary from AudioFileProperties.
/// @param properties Shared pointer to AudioFileProperties.
/// @returns NSDictionary containing AVFoundation audio file settings.
NSDictionary *getFileSettings(const std::shared_ptr<AudioFileProperties> &properties)
{
  AudioFormatID format = getFormat(properties);
  NSMutableDictionary *settings = [NSMutableDictionary dictionary];

  settings[AVFormatIDKey] = @(format);
  settings[AVSampleRateKey] = @(properties->sampleRate);
  settings[AVNumberOfChannelsKey] = @(properties->channelCount);
  settings[AVEncoderAudioQualityKey] = @(getQuality(properties));

  if (format == kAudioFormatMPEG4AAC) {
    settings[AVEncoderBitRateKey] = @(properties->bitRate);
  }

  if (format == kAudioFormatLinearPCM) {
    NSInteger bitDepth = getBitDepth(properties);

    settings[AVLinearPCMBitDepthKey] = @(bitDepth);
    settings[AVLinearPCMIsFloatKey] = @(bitDepth == 32);
    settings[AVLinearPCMIsBigEndianKey] = @(NO);
    settings[AVLinearPCMIsNonInterleaved] = @(NO);
  }

  if (format == kAudioFormatFLAC) {
    settings[@"FLACCompressionLevel"] = @(getFlacCompressionLevel(properties));
  }

  return settings;
}

NSURL *getFileURL(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride)
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

  NSString *fileNamePrefix = [NSString stringWithUTF8String:properties->fileNamePrefix.c_str()];
  NSString *timestamp = getTimestampString();
  NSString *fileExtension = getFileExtension(properties);

  NSString *fileName = fileNameOverride.length() > 0
      ? [NSString stringWithFormat:@"%@.%@",
                                   [NSString stringWithUTF8String:fileNameOverride.c_str()],
                                   fileExtension]
      : [NSString stringWithFormat:@"%@_%@.%@", fileNamePrefix, timestamp, fileExtension];

  return [directoryURL URLByAppendingPathComponent:fileName];
}

NSSearchPathDirectory getDirectory(const std::shared_ptr<AudioFileProperties> &properties)
{
  switch (properties->directory) {
    case AudioFileProperties::FileDirectory::Document:
      return NSDocumentDirectory;

    case AudioFileProperties::FileDirectory::Cache:
      return NSCachesDirectory;

    default:
      return NSCachesDirectory;
  }
}

NSString *getTimestampString()
{
  NSDateFormatter *fmt = [[NSDateFormatter alloc] init];
  fmt.locale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
  fmt.timeZone = [NSTimeZone timeZoneWithAbbreviation:@"UTC"]; // or local if you prefer
  fmt.dateFormat = @"yyyyMMdd_HHmmss_SSS";
  return [fmt stringFromDate:[NSDate date]];
}

} // namespace audioapi::ios::fileoptions
