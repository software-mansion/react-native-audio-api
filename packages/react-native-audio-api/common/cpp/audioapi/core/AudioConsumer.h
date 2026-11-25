#pragma once

#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <string>

namespace audioapi {

/// Receives a stream of audio frames: a file writer, a JS callback, a bridge into a graph.
/// Called on a worker thread, never on the real-time thread and never concurrently, except
/// invalidate(), which comes from the JS thread.
class AudioConsumer {
 public:
  AudioConsumer() = default;
  virtual ~AudioConsumer() = default;
  DELETE_COPY_AND_MOVE(AudioConsumer);

  /// A stream in @p format starts, or the consumer joins a running one.
  virtual Result<NoneType, std::string> prepare(const StreamFormat &format) = 0;

  /// The next @p numFrames frames of the stream; @p frames is valid only for the call. Keep it
  /// short: a slow consume() delays the other consumers.
  virtual Result<NoneType, std::string> consume(const AudioBuffer &frames, size_t numFrames) = 0;

  /// The stream continues in @p format. Closes and prepares again by default.
  virtual Result<NoneType, std::string> reprepare(const StreamFormat &format) {
    close();
    return prepare(format);
  }

  /// The stream ended or the consumer was removed; it may be prepared again later.
  virtual void close() = 0;

  /// Drops every JS reference it holds. The consumer may still be fed afterwards.
  virtual void invalidate() {}
};

} // namespace audioapi
