#pragma once

#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <string>

namespace audioapi {

class RecorderOutput {
 public:
  RecorderOutput() = default;
  virtual ~RecorderOutput() = default;
  DELETE_COPY_AND_MOVE(RecorderOutput);

  /// A session starts, or the output joins one, and the input delivers @p format from now on.
  virtual Result<NoneType, std::string> prepare(const StreamFormat &format) = 0;

  /// The recorder's effects worker. The first @p numFrames frames of @p frames, one channel per
  /// input channel, are the next ones the input delivered; valid only for the call. An error is
  /// reported as the recorder's error event; the output stays open and gets the next frames.
  virtual Result<NoneType, std::string> write(const AudioBuffer &frames, size_t numFrames) = 0;

  /// The input switched to @p format mid-session; every frame of the old format was written.
  /// Closes and prepares again, so whatever the output buffers is handed on first; an output that
  /// can carry its state across the change overrides it. One that fails stays closed.
  virtual Result<NoneType, std::string> reprepare(const StreamFormat &format) {
    close();
    return prepare(format);
  }

  /// The session ended or the output was removed: hands on or drops what it still buffers.
  virtual void close() = 0;

  /// Drops every handler, function or runtime it holds.
  /// The output may still be prepared and written to afterwards.
  virtual void invalidate() {}
};

} // namespace audioapi
