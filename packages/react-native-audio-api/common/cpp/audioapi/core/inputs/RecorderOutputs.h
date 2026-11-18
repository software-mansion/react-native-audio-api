#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEvent.h>
#include <audioapi/utils/AudioBufferOffloader.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

class AudioFileWriter;
class AudioFileProperties;
class RecorderOutput;
struct PlatformFileBackend;
template <AudioEvent Event>
class EventCaller;

/// @brief What a recorder writes its frames to: this session's file writer and the RecorderOutput
/// list. Each is prepared for the stream format a session delivers and fed by one effects worker,
/// so the audio thread only copies a buffer into a pool slot and hands it on. The writer is a
/// RecorderOutput too but kept apart and typed: a new one is made for every session, and it
/// leaves with detach() for finalize() to close, which reports its files.
///
/// Control calls are not synchronized with each other: the owner serializes them, and disarms its
/// input before prepare(), reprepare() and detach(), the only calls that start or stop the worker.
/// write() runs on the audio thread alongside the rest and never blocks.
class RecorderOutputs final {
 public:
  /// Closing the file can block, so finalize() runs with no mutex held.
  struct Detached {
    std::shared_ptr<AudioFileWriter> fileWriter;
  };

  /// Writes files through @p fileBackend, which must outlive the outputs. The file writer and the
  /// listed outputs' failed writes are reported through @p errorEvent.
  RecorderOutputs(
      std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent,
      const PlatformFileBackend &fileBackend);
  DELETE_COPY_AND_MOVE(RecorderOutputs);
  ~RecorderOutputs();

  /// Audio thread. @p channels holds one pointer per channel of the prepared format, each to
  /// @p numFrames float32 samples, valid only for the call. A buffer that finds no free pool slot
  /// reaches no output.
  void write(const float *const *channels, int numFrames);

  /// A session starts delivering @p format: prepares a new file writer when the file is enabled,
  /// then every listed output, then starts the worker. Stops at the first output that fails,
  /// leaving the ones before it prepared; detach() closes them again.
  Result<NoneType, std::string> prepare(const StreamFormat &format);

  /// The session now delivers @p format. The worker first hands every queued buffer of the old
  /// format to the outputs; then every prepared output, the file writer included, is reprepared.
  /// Every one is attempted, so none is left prepared for the old format: one that fails stays
  /// off until the next prepare(). Returns the failures, joined.
  Result<NoneType, std::string> reprepare(const StreamFormat &format);

  /// The session ended: the worker hands on what it holds and stops, the listed outputs are
  /// closed and stay listed for the next prepare(), and the writer leaves, unclosed, with this
  /// session's file.
  Detached detach();

  /// Closes what detach() or disableFile() handed over. Run with no recorder mutex held: closing
  /// the writer joins its worker thread. The file URIs are collected only after that join, once a
  /// rotation in flight has reported its file.
  static Result<FileInfo, std::string> finalize(Detached detached);

  /// Takes effect at the next prepare(). Fails on properties this platform cannot write.
  Result<NoneType, std::string> enableFile(std::shared_ptr<AudioFileProperties> properties);
  /// Takes effect at once: hands over the writer, with the file it has open, for finalize().
  /// Buffers still queued for the worker are not written to it.
  Detached disableFile();

  /// With @p currentFormat, a session is running and @p output is prepared for it at once; one
  /// that cannot be prepared, or is listed already, is not added.
  Result<NoneType, std::string> add(
      const std::shared_ptr<RecorderOutput> &output,
      const std::optional<StreamFormat> &currentFormat);
  /// Closes @p output if it was prepared. Not listed: nothing happens.
  void remove(const std::shared_ptr<RecorderOutput> &output);

 private:
  template <typename Output>
  struct PreparedOutput {
    std::shared_ptr<Output> output;
    bool isPrepared = false;
  };
  using ListedOutput = PreparedOutput<RecorderOutput>;

  Result<NoneType, std::string> startWorker(const StreamFormat &format);
  /// Worker thread.
  void deliver(PendingFrames pending);

  /// Guards the members below between control calls and the worker; never taken on the audio
  /// thread.
  std::mutex outputsMutex_;
  /// What the next session's file is written with; null while the file is disabled.
  std::shared_ptr<AudioFileProperties> fileProperties_;
  /// This session's writer; empty without a session or with the file disabled.
  PreparedOutput<AudioFileWriter> file_;
  std::vector<ListedOutput> outputs_;

  std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent_;
  const PlatformFileBackend &fileBackend_;

  /// Runs only while a session is prepared; started and stopped only while the input is disarmed.
  /// Last, so its worker is joined before the outputs it delivers to go.
  AudioBufferOffloader worker_{[this](PendingFrames pending) { deliver(std::move(pending)); }};
};

} // namespace audioapi
