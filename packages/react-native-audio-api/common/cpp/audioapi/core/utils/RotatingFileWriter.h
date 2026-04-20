#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <functional>
#include <memory>
#include <string>

namespace audioapi {

class RotatingFileWriter {
 public:
  virtual ~RotatingFileWriter() = default;
  using WriterFactory =
      std::function<std::shared_ptr<AudioFileWriter>(const std::shared_ptr<AudioFileProperties> &)>;
  using OnSegmentFileOpenedCallback = std::function<void(const std::string &)>;

  RotatingFileWriter(
      size_t rotateIntervalBytes,
      WriterFactory writerFactory,
      OnSegmentFileOpenedCallback onSegmentFileOpened);

  CloseFileResult closeFile();
  virtual void rotateFiles();

 protected:
  static constexpr int FILE_SIZE_CHECK_WRITE_INTERVAL = 10;

  WriterFactory writerFactory_;
  OnSegmentFileOpenedCallback onSegmentFileOpened_;
  size_t rotateIntervalBytes_;
  size_t writesSinceLastCheck_ = 0;
  std::shared_ptr<AudioFileWriter> currentWriter_;
  std::string baseFileName_;

  double cumulativeSizeMB_{0.0};
  double cumulativeDurationSec_{0.0};
};

} // namespace audioapi
