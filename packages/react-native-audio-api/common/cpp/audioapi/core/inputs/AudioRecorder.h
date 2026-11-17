#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace audioapi {

class AudioFileWriter;
class AudioFileProperties;
class AudioRecorderCallback;
class IAudioEventHandlerRegistry;
class RecorderAdapterNode;
struct AudioRecorderOptions;
struct PlatformFileBackend;

/// Platform-independent half of a microphone recorder: subclasses own the platform input
/// stream; the file writer, the JS callback and the adapter node are managed here, from
/// preparing them for a session to closing them once it ends.
class AudioRecorder {
 public:
  /// Writes files through @p fileBackend, which must outlive the recorder; nullptr selects the
  /// platform's (osFileBackend()).
  explicit AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const PlatformFileBackend *fileBackend = nullptr);
  DELETE_COPY_AND_MOVE(AudioRecorder);
  /// Closes any output still prepared. A subclass stops its session in its own destructor, so
  /// normally none is.
  virtual ~AudioRecorder();

  virtual Result<NoneType, std::string> start() = 0;
  virtual Result<FileInfo, std::string> stop() = 0;

  Result<NoneType, std::string> enableFileOutput(std::shared_ptr<AudioFileProperties> properties);
  void disableFileOutput();

  virtual void pause() = 0;
  /// @brief Resumes a paused recording.
  /// @return The platform's reason when it refused; Ok when the recorder resumed or was not
  /// paused.
  virtual Result<NoneType, std::string> resume() = 0;

  /// @brief Moves a recording or paused session onto the input device the platform
  /// currently selects.
  virtual Result<NoneType, std::string> rerouteInput() = 0;

  /// @p node must carry a RecorderAdapterNode; the handle keeps it alive while connected.
  void connect(const std::shared_ptr<utils::graph::NodeHandle> &node);
  void disconnect();

  Result<NoneType, std::string> setOnAudioReadyCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  void clearOnAudioReadyCallback();

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();

  virtual double getCurrentDuration() const;

  bool usesCallback() const;
  bool usesFileOutput() const;
  bool isConnected() const;

  virtual bool isRecording() const = 0;
  virtual bool isPaused() const = 0;
  virtual bool isIdle() const = 0;

  RecorderState getState() const;

  /// Seconds between a sample reaching the microphone and the recorder receiving it; 0 while
  /// idle.
  [[nodiscard]] double getInputLatency() const;

 protected:
  /// @brief Lifecycle of one recorder output: the file writer, the JS callback, or the adapter
  /// node.
  enum class OutputState : uint8_t {
    /// Not requested.
    Disabled,
    /// Requested, but not yet prepared for the current stream format. The next start(), or an
    /// input format change, prepares it.
    Requested,
    /// Prepared for the current stream format; the audio thread may feed it.
    Active,
  };

  /// One output of the recorder and what guards it.
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

    void deactivate() {
      auto expected = OutputState::Active;
      state.compare_exchange_strong(expected, OutputState::Requested, std::memory_order_acq_rel);
    }
  };

  /// Closing these can block, so it happens in finalizeSideEffects() with no mutex held.
  struct DetachedSideEffects {
    std::shared_ptr<AudioFileWriter> fileWriter;
    std::shared_ptr<AudioRecorderCallback> dataCallback;
    std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
    std::vector<std::string> fileUris;
  };

  /// Audio thread. @p channels holds one pointer per stream channel, each to numFrames float32
  /// samples, valid only for the call. Every consumer tryLocks its mutex and drops the buffer
  /// rather than block.
  void onAudioFrames(const float *const *channels, int numFrames);

  /// JS thread. Fails while the input is unavailable, which on iOS happens between a route
  /// change and the engine settling on the replacement format.
  [[nodiscard]] virtual Result<StreamFormat, std::string> resolveStreamFormat() const = 0;

  /// The platform's input latency for a live session. @p callbackSeconds is how long the last
  /// delivered buffer lasted, or 0 before the first one.
  [[nodiscard]] virtual double platformInputLatency(double callbackSeconds) const = 0;

  /// Opens the session's file for @p format. The caller must hold file_.mutex.
  Result<NoneType, std::string> prepareFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties,
      const StreamFormat &format);

  /// The caller must hold adapter_.mutex.
  Result<NoneType, std::string> prepareAdapterNode(const StreamFormat &format);

  /// For a session that is starting; a live one follows an input change through
  /// reprepareOutputs(). Stops at the first output that fails, leaving the ones before it
  /// prepared: a start that fails ends with detachSideEffects() and finalizeSideEffects(), which
  /// close them again. The caller must hold every output's mutex.
  Result<NoneType, std::string> prepareOutputs(const StreamFormat &format);

  /// Follows an input format change mid-session: the file writer keeps its file, the callback
  /// and the adapter node are rebuilt. Every output is attempted, so none is left prepared for
  /// the old format: one that fails stays off until the next start(). Returns the failures,
  /// joined. Nothing may deliver frames in the old format once this runs.
  /// The caller must hold every output's mutex.
  Result<NoneType, std::string> reprepareOutputs(const StreamFormat &format);

  /// Delivers @p message to the JS error callback, if one is registered.
  void reportError(const std::string &message);

  /// Payload of @p handle, or nullptr when not connected. Valid exactly as long as the handle
  /// is held, so callers keep the handle (or adapter_.mutex) for the pointer's lifetime.
  static RecorderAdapterNode *adapterNodeOf(
      const std::shared_ptr<utils::graph::NodeHandle> &handle);

  /// Stops the audio thread from touching the side effects before they are closed.
  /// The caller must hold every output's mutex.
  DetachedSideEffects detachSideEffects();

  /// Must run with no recorder mutex held: closing the writer joins its worker thread. The
  /// file URIs are collected only after that join, once a rotation in flight has reported its file.
  static Result<FileInfo, std::string> finalizeSideEffects(DetachedSideEffects sideEffects);

  std::atomic<RecorderState> state_{RecorderState::Idle};

  OutputSlot<AudioFileWriter> file_;
  OutputSlot<AudioRecorderCallback> callback_;
  /// Its target carries a RecorderAdapterNode; see adapterNodeOf().
  OutputSlot<utils::graph::NodeHandle> adapter_;

  std::mutex errorCallbackMutex_;
  std::atomic<uint64_t> errorCallbackId_{0};

  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  std::shared_ptr<AudioFileProperties> fileProperties_ = nullptr;
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  /// Sample rate of the live input stream, published for readers off the JS thread.
  std::atomic<float> streamSampleRate_{0.0F};

 private:
  /// The caller must hold callback_.mutex. A callback already prepared is flushed and torn
  /// down first.
  Result<NoneType, std::string> prepareCallback(const StreamFormat &format);

  /// The caller must hold file_.mutex.
  Result<NoneType, std::string> reprepareFileWriter(const StreamFormat &format);

  /// The caller must hold adapter_.mutex.
  Result<NoneType, std::string> reprepareAdapterNode(const StreamFormat &format);

  const PlatformFileBackend &fileBackend_;
};

/// Builds the platform's AudioRecorder.
std::shared_ptr<AudioRecorder> createPlatformAudioRecorder(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const AudioRecorderOptions &options);

} // namespace audioapi
