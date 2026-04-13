#pragma once

#include <audioapi/core/utils/RotatingFileWriter.h>
#include <audioapi/ios/core/utils/IOSFileWriter.h>

#include <functional>
#include <memory>
#include <string>
#include "audioapi/utils/AudioFileProperties.h"

namespace audioapi {

#ifndef __OBJC__
typedef struct objc_object AVAudioFormat;
#endif

class IOSRotatingFileWriter : public IOSFileWriter, public RotatingFileWriter {
 public:
  using WriterFactory =
      std::function<std::shared_ptr<AudioFileWriter>(const std::shared_ptr<AudioFileProperties> &)>;

  IOSRotatingFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      size_t rotateIntervalBytes,
      WriterFactory writerFactory,
      std::function<void(const std::string &)> onSegmentFileOpened = {});

  OpenFileResult openFile(
      AVAudioFormat *streamFormat,
      size_t streamMaxBufferSizeInFrames,
      const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;
  [[nodiscard]] double getCurrentDuration() const override;

  void writeAudioData(AudioDataType data, int numFrames) override;

 private:
  void rotateFiles() override;
  OpenFileResult openInnerWriter();
  AVAudioFormat *streamFormat_;
  size_t streamMaxBufferSizeInFrames_;
  std::shared_ptr<AudioFileProperties> fileProperties_;
};

} // namespace audioapi
