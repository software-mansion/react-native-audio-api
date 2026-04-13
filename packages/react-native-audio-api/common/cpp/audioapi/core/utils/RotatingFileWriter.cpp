#include <audioapi/core/utils/RotatingFileWriter.h>

#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace audioapi {

RotatingFileWriter::RotatingFileWriter(
    size_t rotateIntervalBytes,
    WriterFactory writerFactory,
    OnSegmentFileOpenedCallback onSegmentFileOpened)
    : writerFactory_(std::move(writerFactory)),
      rotateIntervalBytes_(rotateIntervalBytes),
      onSegmentFileOpened_(std::move(onSegmentFileOpened)) {}

CloseFileResult RotatingFileWriter::closeFile() {
  if (currentWriter_ == nullptr) {
    return CloseFileResult::Err("No file open");
  }
  auto closeResult = currentWriter_->closeFile();
  if (closeResult.is_err()) {
    return CloseFileResult::Err(closeResult.unwrap_err());
  }
  const auto &lastTuple = closeResult.unwrap();
  const double totalSizeMB = cumulativeSizeMB_ + std::get<0>(lastTuple);
  const double totalDurationSec = cumulativeDurationSec_ + std::get<1>(lastTuple);
  cumulativeSizeMB_ = 0.0;
  cumulativeDurationSec_ = 0.0;
  return CloseFileResult::Ok({totalSizeMB, totalDurationSec});
}

void RotatingFileWriter::rotateFiles() {
  auto rotatedClose = currentWriter_->closeFile();
  if (rotatedClose.is_ok()) {
    const auto &t = rotatedClose.unwrap();
    cumulativeSizeMB_ += std::get<0>(t);
    cumulativeDurationSec_ += std::get<1>(t);
  }
}

} // namespace audioapi
