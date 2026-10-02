#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace audioapi {

/// @brief One open file of a recording session: the encoder writing it, its path, and how much
/// audio it holds so far.
///
/// Not thread-safe. AudioFileWriter keeps it behind its file mutex, which both the JS thread
/// and the writer's worker thread take.
class RecordingFile final {
 public:
  using RetargetEncoder = std::function<OpenEncoderResult(AudioEncoder &, const StreamFormat &)>;

  /// Opens @p encoder on @p path for input in @p inputFormat.
  [[nodiscard]] static Result<std::unique_ptr<RecordingFile>, std::string> open(
      std::unique_ptr<AudioEncoder> encoder,
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      const std::string &path);

  DELETE_COPY_AND_MOVE(RecordingFile);
  ~RecordingFile() = default;

  [[nodiscard]] const std::string &path() const;

  /// Counts the frames toward durationSec() only once the encoder accepts them.
  EncodeResult encode(const float *const *channels, int numFrames);

  /// Keeps writing the same file from input in @p inputFormat. The audio encoded so far keeps
  /// counting at the sample rate it arrived in.
  OpenEncoderResult changeInputFormat(
      const StreamFormat &inputFormat,
      const RetargetEncoder &retargetEncoder);

  /// Of the audio encoded so far, measured at the input sample rate.
  [[nodiscard]] double durationSec() const;
  [[nodiscard]] size_t sizeBytes() const;

  /// Finalizes the file. Returns {sizeMB, durationSeconds} as the encoder reports them.
  CloseEncoderResult close();
  /// Closes the encoder and deletes the file, for a file that must not outlive a failed open.
  void discard();

 private:
  RecordingFile(std::unique_ptr<AudioEncoder> encoder, std::string path, float inputSampleRate);

  std::unique_ptr<AudioEncoder> encoder_;
  std::string path_;
  float inputSampleRate_;
  size_t framesInCurrentFormat_{0};
  double earlierFormatsDurationSec_{0.0};
};

} // namespace audioapi
