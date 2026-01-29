#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <functional>
#include <memory>
#include <string>

namespace audioapi {

class RotatingFileWriter : public AudioFileWriter {
 public:
  using WriterFactory =
      std::function<std::shared_ptr<AudioFileWriter>(const std::shared_ptr<AudioFileProperties> &)>;

  RotatingFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      size_t rotateIntervalBytes,
      WriterFactory writerFactory);

  ~RotatingFileWriter() override = default;

  // AudioFileWriter overrides
  OpenFileResult openFile() override;
  CloseFileResult closeFile() override;
  std::string getFilePath() const override;
  bool writeAudioData(AudioDataType data, int numFrames) override;
  double getCurrentDuration() const override;
  size_t getFileSizeBytes() const override;

  // Rotating logic
  void rotateFiles();

 private:
  std::shared_ptr<AudioFileProperties> createRotatedProperties();
  void openNewFile();

  WriterFactory writerFactory_;
  size_t rotateIntervalBytes_;
  size_t currentFileBytes_ = 0;
  size_t writesSinceLastCheck_ = 0;
  std::shared_ptr<AudioFileWriter> currentWriter_;
  std::string baseFileName_;
};

} // namespace audioapi
