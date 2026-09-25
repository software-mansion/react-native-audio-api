#import <Foundation/Foundation.h>

#include <audioapi/ios/core/utils/IOSFilePath.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <memory>
#include <string>

namespace audioapi::ios_filepath {

namespace {

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

} // namespace

ResolveFilePathResult resolveFilePath(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileName)
{
  @autoreleasepool {
    NSURL *fileURL = getFileURL(properties, fileName);
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
