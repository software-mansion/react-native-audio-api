// FileOptions.h names AudioFormatID / NSSearchPathDirectory without declaring them,
// so its AVFoundation prerequisites have to come first.
#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/FileOptions.h>
#include <audioapi/ios/core/utils/IOSFilePath.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <string>

namespace audioapi::ios_filepath {

ResolveFilePathResult resolveFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride)
{
  @autoreleasepool {
    NSURL *fileURL = ios::fileoptions::getFileURL(properties, fileNameOverride);
    if (fileURL == nil) {
      return ResolveFilePathResult::Err("Could not resolve an output path for the recording");
    }

    NSString *path = [fileURL path];
    if (path == nil) {
      return ResolveFilePathResult::Err(
          std::string("Output URL has no file path: ") + [[fileURL absoluteString] UTF8String]);
    }

    return ResolveFilePathResult::Ok(std::string([path UTF8String]));
  }
}

} // namespace audioapi::ios_filepath
