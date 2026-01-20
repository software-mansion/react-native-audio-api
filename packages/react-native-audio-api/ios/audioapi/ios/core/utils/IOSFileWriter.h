#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/Result.hpp>
#include <memory>
#include <string>
#include <tuple>

#ifndef __OBJC__ // when compiled as C++
typedef struct objc_object NSURL;
typedef struct objc_object NSString;
typedef struct objc_object AVAudioFile;
typedef struct objc_object AVAudioFormat;
typedef struct objc_object AudioBufferList;
typedef struct objc_object AVAudioConverter;
#endif // __OBJC__

namespace audioapi {

class AudioFileProperties;
class AudioEventHandlerRegistry;

class IOSFileWriter : public AudioFileWriter {
 public:
  IOSFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties);
  ~IOSFileWriter();

  Result<std::string, std::string> openFile(
      AVAudioFormat *bufferFormat,
      size_t maxInputBufferLength,
      const std::string &fileNameOverride);
  Result<std::tuple<double, double>, std::string> closeFile() override;

  bool writeAudioData(const AudioBufferList *audioBufferList, int numFrames);
  double getCurrentDuration() const override;

  std::string getFilePath() const override;

 protected:
  size_t converterInputBufferSize_;
  size_t converterOutputBufferSize_;

  AVAudioFile *audioFile_;
  AVAudioFormat *bufferFormat_;
  AVAudioConverter *converter_;
  NSURL *fileURL_;

  AVAudioPCMBuffer *converterInputBuffer_;
  AVAudioPCMBuffer *converterOutputBuffer_;
};

} // namespace audioapi
