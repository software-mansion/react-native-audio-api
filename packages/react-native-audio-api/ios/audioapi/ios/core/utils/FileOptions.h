#pragma once

#ifndef __OBJC__ // when compiled as C++
typedef struct objc_object AVAudioFile;
typedef struct objc_object NSURL;
typedef struct objc_object AudioBufferList;
#endif // __OBJC__

#include <memory>

namespace audioapi {

class AudioFileProperties;

namespace ios::fileoptions {

AudioFormatID getFormat(const std::shared_ptr<AudioFileProperties> &properties);
NSInteger getQuality(const std::shared_ptr<AudioFileProperties> &properties);
NSInteger getFlacCompressionLevel(const std::shared_ptr<AudioFileProperties> &properties);
NSString *getFileExtension(const std::shared_ptr<AudioFileProperties> &properties);
NSInteger getBitDepth(const std::shared_ptr<AudioFileProperties> &properties);
float getSampleRate(const std::shared_ptr<AudioFileProperties> &properties);

NSDictionary *getFileSettings(const std::shared_ptr<AudioFileProperties> &properties);
NSURL *getFileURL(
    const std::shared_ptr<AudioFileProperties> &properties,
    const std::string &fileNameOverride);
NSSearchPathDirectory getDirectory(const std::shared_ptr<AudioFileProperties> &properties);

NSString *getDateString();
NSString *getTimestampString();

} // namespace ios::fileoptions

} // namespace audioapi
