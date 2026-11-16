#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace audioapi {

class AudioFileWriter;
class AudioFileProperties;
class AudioRecorderCallback;
class IAudioEventHandlerRegistry;
class RecorderAdapterNode;
struct PlatformFileBackend;

/// @brief What a recorder writes its frames to: the file writer, the JS callback and the adapter
/// node. Each is requested by the app, prepared for the stream format a session delivers, and
/// fed from the audio thread.
///
/// Control calls are not synchronized with each other: the owner serializes them, and disarms its
/// input before follow() so that no frames in the old format arrive while the outputs change.
/// write() runs on the audio thread alongside them and never blocks.
class RecorderOutputs final {
 public:
  /// Closing these can block, so finalize() runs with no mutex held.
  struct Detached {
    std::shared_ptr<AudioFileWriter> fileWriter;
    std::shared_ptr<AudioRecorderCallback> dataCallback;
    std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
  };

  /// Writes files through @p fileBackend, which must outlive the outputs.
  RecorderOutputs(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const PlatformFileBackend &fileBackend);
  DELETE_COPY_AND_MOVE(RecorderOutputs);
  ~RecorderOutputs() = default;

  /// Audio thread. @p channels holds one pointer per channel of the prepared format, each to
  /// @p numFrames float32 samples, valid only for the call. An output busy with a control call
  /// drops the buffer.
  void write(const float *const *channels, int numFrames);

  /// A session starts delivering @p format: prepares every requested output. Stops at the first
  /// output that fails, leaving the ones before it prepared; detach() closes them again.
  Result<NoneType, std::string> prepare(const StreamFormat &format);

  /// The session now delivers @p format: the file writer keeps its file, the callback and the
  /// adapter node are rebuilt. Every prepared output is attempted, so none is left prepared for
  /// the old format: one that fails stays off until the next prepare(). Returns the failures,
  /// joined.
  Result<NoneType, std::string> follow(const StreamFormat &format);

  /// The session ended: takes every output away from write(). The writer leaves with this
  /// session's file; the callback and the adapter node stay requested, so the next prepare()
  /// feeds the same ones again.
  Detached detach();

  /// Closes what detach(), disableFile() or disconnect() handed over. Run with no recorder
  /// mutex held: closing the writer joins its worker thread. The file URIs are collected only
  /// after that join, once a rotation in flight has reported its file.
  static Result<FileInfo, std::string> finalize(Detached detached);

  /// Takes effect at the next prepare(). Fails on properties this platform cannot write.
  Result<NoneType, std::string> enableFile(std::shared_ptr<AudioFileProperties> properties);
  /// Hands over the writer, with the file it has open, for finalize().
  Detached disableFile();

  /// @p node must carry a RecorderAdapterNode; the handle keeps it alive while connected. With
  /// @p currentFormat, a session is running and the node is prepared for it at once.
  void connect(
      const std::shared_ptr<utils::graph::NodeHandle> &node,
      const std::optional<StreamFormat> &currentFormat);
  /// Hands over the node, if it was prepared, for finalize().
  Detached disconnect();

  /// With @p currentFormat, a session is running and the callback is prepared for it at once;
  /// a callback that cannot be prepared is not kept.
  Result<NoneType, std::string> setCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId,
      const std::optional<StreamFormat> &currentFormat);
  void clearCallback();

  /// 0 clears it. The file writer and the callback report their errors to it.
  void setErrorCallbackId(uint64_t callbackId);
  [[nodiscard]] uint64_t errorCallbackId() const;

  /// Across every file of the session; 0 when no file is being written.
  [[nodiscard]] double currentFileDuration() const;

 private:
  enum class OutputState : uint8_t {
    Disabled,
    Requested,
    Active,
  };

  /// One output and what guards it.
  template <typename Target>
  struct OutputSlot {
    std::atomic<OutputState> state{OutputState::Disabled};
    mutable std::mutex mutex;
    std::shared_ptr<Target> target;

    [[nodiscard]] bool isActive() const {
      return state.load(std::memory_order_acquire) == OutputState::Active;
    }

    [[nodiscard]] bool isWanted() const {
      return state.load(std::memory_order_acquire) != OutputState::Disabled;
    }

    void set(OutputState newState) {
      state.store(newState, std::memory_order_release);
    }

    /// Active becomes Requested; Disabled stays Disabled, so an output that was never requested
    /// cannot be switched on by a failed prepare.
    void deactivate() {
      auto expected = OutputState::Active;
      state.compare_exchange_strong(expected, OutputState::Requested, std::memory_order_acq_rel);
    }
  };

  Result<NoneType, std::string> prepareFileWriter(const StreamFormat &format);
  Result<NoneType, std::string> reprepareFileWriter(const StreamFormat &format);
  Result<NoneType, std::string> prepareCallback(const StreamFormat &format);
  Result<NoneType, std::string> prepareAdapterNode(const StreamFormat &format);

  static RecorderAdapterNode *adapterNodeOf(
      const std::shared_ptr<utils::graph::NodeHandle> &handle);

  OutputSlot<AudioFileWriter> file_;
  OutputSlot<AudioRecorderCallback> callback_;
  OutputSlot<utils::graph::NodeHandle> adapter_;

  /// What the next session's file is written with.
  std::shared_ptr<AudioFileProperties> fileProperties_;
  std::atomic<uint64_t> errorCallbackId_{0};

  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  const PlatformFileBackend &fileBackend_;
};

} // namespace audioapi
