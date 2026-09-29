#include <audioapi/core/utils/RecordingFile.h>
#include <audioapi/utils/FileSystem.hpp>

#include <memory>
#include <string>
#include <utility>

namespace audioapi {

RecordingFile::RecordingFile(
    std::unique_ptr<AudioEncoder> encoder,
    std::string path,
    float inputSampleRate)
    : encoder_(std::move(encoder)), path_(std::move(path)), inputSampleRate_(inputSampleRate) {}

Result<std::unique_ptr<RecordingFile>, std::string> RecordingFile::open(
    std::unique_ptr<AudioEncoder> encoder,
    const StreamFormat &inputFormat,
    const EncoderOutputSpec &outputSpec,
    const std::string &path) {
  auto openResult = encoder->open(inputFormat, outputSpec, path);
  if (openResult.is_err()) {
    return Err(openResult.unwrap_err());
  }
  return Ok(
      std::unique_ptr<RecordingFile>(
          new RecordingFile(std::move(encoder), path, inputFormat.sampleRate)));
}

const std::string &RecordingFile::path() const {
  return path_;
}

EncodeResult RecordingFile::encode(const float *const *channels, int numFrames) {
  auto result = encoder_->encode(channels, numFrames);
  if (result.is_ok()) {
    framesInCurrentFormat_ += static_cast<size_t>(numFrames);
  }
  return result;
}

OpenEncoderResult RecordingFile::changeInputFormat(
    const StreamFormat &inputFormat,
    const RetargetEncoder &retargetEncoder) {
  earlierFormatsDurationSec_ = durationSec();
  framesInCurrentFormat_ = 0;
  inputSampleRate_ = inputFormat.sampleRate;
  return retargetEncoder(*encoder_, inputFormat);
}

double RecordingFile::durationSec() const {
  if (inputSampleRate_ <= 0) {
    return earlierFormatsDurationSec_;
  }
  return earlierFormatsDurationSec_ +
      static_cast<double>(framesInCurrentFormat_) / static_cast<double>(inputSampleRate_);
}

size_t RecordingFile::sizeBytes() const {
  return encoder_->getFileSizeBytes();
}

CloseEncoderResult RecordingFile::close() {
  return encoder_->close();
}

void RecordingFile::discard() {
  // Whatever the encoder reports about a file that is about to be deleted is of no use.
  (void)encoder_->close();
  file_system::removeFile(path_);
}

} // namespace audioapi
