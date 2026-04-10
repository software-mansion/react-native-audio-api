#pragma once

#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/core/utils/RotatingFileWriter.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace audioapi {

class AndroidRotatingFileWriter : public AndroidFileWriterBackend, public RotatingFileWriter {
 public:
  using WriterFactory =
      std::function<std::shared_ptr<AudioFileWriter>(const std::shared_ptr<AudioFileProperties> &)>;

  AndroidRotatingFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      size_t rotateIntervalBytes,
      WriterFactory writerFactory,
      std::function<void(const std::string &)> onSegmentFileOpened = {});

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t streamMaxBufferSizeInFrames,
      const std::string &fileNameOverride) override;

  CloseFileResult closeFile() override;
  [[nodiscard]] double getCurrentDuration() const override;
  void writeAudioData(AudioDataType data, int numFrames) override;

 private:
  void taskOffloaderFunction(WriterData data) override;

  void rotateFiles() override;
  OpenFileResult openInnerWriter();
  float streamSampleRate_;
  int32_t streamChannelCount_;
  int32_t streamMaxBufferSize_;
};

} // namespace audioapi
