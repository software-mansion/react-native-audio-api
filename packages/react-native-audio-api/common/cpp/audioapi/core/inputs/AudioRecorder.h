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
#include <optional>
#include <string>
#include <vector>

namespace audioapi {

class AudioFileWriter;
class AudioFileProperties;
class AudioRecorderCallback;
class IAudioEventHandlerRegistry;
class IRecorderInput;
class IRecordingSessionRegistry;
class RecorderAdapterNode;
class RecorderInputGate;
struct PlatformFileBackend;

/// @brief A microphone recorder: the session state machine and its outputs (the file writer, the
/// JS callback and the adapter node), shared by every platform. The platform's part is the
/// capture stream behind IRecorderInput.
class AudioRecorder final : public std::enable_shared_from_this<AudioRecorder> {
 public:
  /// Writes files through @p fileBackend; nullptr selects the platform's (osFileBackend()).
  /// Claims its sessions from @p sessionRegistry and reports them back to it; nullptr for a
  /// recorder that shares the process with no other. Both must outlive the recorder.
  AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      std::shared_ptr<IRecorderInput> input,
      const PlatformFileBackend *fileBackend = nullptr,
      IRecordingSessionRegistry *sessionRegistry = nullptr);

  DELETE_COPY_AND_MOVE(AudioRecorder);
  ~AudioRecorder();

  /// Fails without starting while another recorder holds the process's session.
  Result<NoneType, std::string> start();
  Result<FileInfo, std::string> stop();
  void pause();
  /// Ok when the session is already recording. Fails while idle, like MediaRecorder.resume(),
  /// and with the platform's reason when the input refuses to start.
  Result<NoneType, std::string> resume();

  /// @brief Reopens capture on the input the platform selects now: after a route change
  /// (Android's AudioInputRouter) or a disconnected stream. A different stream format re-points the
  /// outputs without splitting the file. Does nothing while idle, or when @p openedGeneration is
  /// given and the input has been closed since that opening (see SessionState::inputGeneration).
  ///
  /// When the input cannot be reopened the session ends here, as stop() ends it, and the error is
  /// reported as well as returned.
  Result<NoneType, std::string> rerouteInput(
      std::optional<uint64_t> openedGeneration = std::nullopt);

  /// The platform reconfigured the input it opened at @p openedGeneration on its own, as iOS does
  /// on a route change: the outputs follow the format it delivers now. A failure to follow turns
  /// that output off and is reported; the session goes on. Ignored once that opening has ended.
  void onInputFormatChanged(uint64_t openedGeneration);

  /// The platform gave up recovering the input it opened at @p openedGeneration: reported, the
  /// session goes on as it is. Ignored once that opening has ended.
  void onInputFailed(uint64_t openedGeneration, const std::string &reason);

  /// Takes effect at the next start(): a recording session keeps the file output it started with,
  /// so calling this while not idle fails and changes nothing.
  Result<NoneType, std::string> enableFileOutput(std::shared_ptr<AudioFileProperties> properties);
  /// Closes the file immediately when called mid-recording.
  void disableFileOutput();

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

  /// Delivers @p message to the JS error callback, if one is registered. Any thread.
  void reportError(const std::string &message);

  [[nodiscard]] double getCurrentDuration() const;

  [[nodiscard]] bool usesCallback() const;
  [[nodiscard]] bool usesFileOutput() const;
  [[nodiscard]] bool isConnected() const;

  [[nodiscard]] bool isIdle() const;
  [[nodiscard]] bool isRecording() const;
  [[nodiscard]] bool isPaused() const;
  [[nodiscard]] RecorderState getState() const;

  [[nodiscard]] double getInputLatency() const;

 private:
  friend class RecorderInputGate;
  friend struct AudioRecorderTestPeer;

  /// Where the session is. What holds in each phase:
  ///
  /// | Phase     | input                    | gate     | outputs                           |
  /// |-----------|--------------------------|----------|-----------------------------------|
  /// | Idle      | closed                   | disarmed | not prepared                      |
  /// | Recording | open, started            | armed    | prepared for SessionState::format |
  /// | Paused    | open, stopped by pause() | disarmed | prepared for SessionState::format |
  /// | Rerouting | closed, route on its way | disarmed | prepared for the old format       |
  ///
  /// The system can stop the input of a Recording session on its own (an interrupted iOS
  /// session): the phase stays Recording, and getState() reads the session as paused.
  enum class Phase : uint8_t { Idle, Recording, Paused, Rerouting };

  /// The session as the control paths see it. Guarded by controlMutex_, except for what a member
  /// says about its readers.
  struct SessionState {
    /// Written only by enter(); getState() reads it lock-free.
    std::atomic<Phase> phase{Phase::Idle};
    /// Recording or Paused: what a reroute brings the session back as. pause() can change it
    /// while the route is on its way. getState() reads it lock-free.
    std::atomic<Phase> phaseAfterReroute{Phase::Recording};

    /// Bumped every time the input is closed, so it identifies one opening of the input. A
    /// reroute releases controlMutex_ while it waits for a route and reopens the input afterwards
    /// only if no stop, and no later reroute, closed it meanwhile; events carry the generation of
    /// the opening they come from and are dropped once it has ended.
    uint64_t inputGeneration = 0;

    /// The format the input delivers and the outputs are prepared for; set exactly while not
    /// idle. Written with the gate disarmed; the audio thread reads it while armed.
    std::optional<StreamFormat> format;
    /// format's sample rate, for getters, which do not take controlMutex_. 0 while idle.
    std::atomic<float> sampleRate{0.0F};
    /// Frame count of the last delivered buffer. Written by the audio thread.
    std::atomic<int32_t> lastBufferFrames{0};

    /// Sets format and what getters derive from it; nullopt while idle. The caller holds
    /// controlMutex_, with the gate disarmed.
    void setFormat(const std::optional<StreamFormat> &newFormat);
  };

  /// Lifecycle of one recorder output: the file writer, the JS callback, or the adapter node.
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
    /// Written under controlMutex_ and mutex; the audio thread reads it lock-free to decide
    /// whether to feed the output.
    std::atomic<OutputState> state{OutputState::Disabled};
    /// The audio thread tryLocks it around each buffer and drops the buffer rather than block;
    /// control paths hold it to change target, or what target is prepared for.
    mutable std::mutex mutex;
    std::shared_ptr<Target> target;

    /// Prepared for the current stream format.
    [[nodiscard]] bool isActive() const {
      return state.load(std::memory_order_acquire) == OutputState::Active;
    }

    /// Requested, whether or not it is prepared yet.
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

  /// Closing these can block, so it happens in finalizeSideEffects() with no mutex held.
  struct DetachedSideEffects {
    std::shared_ptr<AudioFileWriter> fileWriter;
    std::shared_ptr<AudioRecorderCallback> dataCallback;
    std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
    std::vector<std::string> fileUris;
  };

  /// Audio thread, through the armed gate only. @p channels holds @p channelCount pointers, each to
  /// numFrames float32 samples, valid only for the call. Every output tryLocks its mutex and
  /// drops the buffer rather than block.
  void onInputFrames(const float *const *channels, int channelCount, int numFrames);

  /// The only place the session changes phase, and the only place that arms the gate: frames
  /// reach the outputs exactly while recording. Entering the current phase again restores that
  /// after a caller had to disarm. The caller holds controlMutex_.
  void enter(Phase phase);

  /// Re-points the outputs at @p format, the one the input delivers now, if it changed. A failure
  /// to follow is reported and the session goes on. The caller holds controlMutex_.
  void followInputFormat(const StreamFormat &format);

  /// Re-points the outputs at @p format when it differs from the one they are prepared for,
  /// disarming the gate first. The caller holds controlMutex_.
  Result<NoneType, std::string> repointOutputsAt(const StreamFormat &format);

  /// start(), once the session is claimed.
  Result<NoneType, std::string> startClaimedSession();

  /// rerouteInput(), leaving a session whose input could not be reopened as it is.
  Result<NoneType, std::string> reopenInput(std::optional<uint64_t> openedGeneration);

  /// Ends the session from any phase: closes the input and takes the outputs away from the audio
  /// thread; finalizing them is the caller's. The caller holds controlMutex_.
  DetachedSideEffects endSession();

  /// The caller must hold file_.mutex.
  Result<NoneType, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties);

  /// The caller must hold adapter_.mutex.
  Result<NoneType, std::string> prepareAdapterNode(const StreamFormat &format);

  /// For a session that is starting; a live one follows an input change through
  /// reprepareOutputs(). The caller must hold every output's mutex.
  Result<NoneType, std::string> prepareOutputs(const StreamFormat &format);

  /// Follows an input format change mid-session: the file writer keeps its file, the callback
  /// and the adapter node are rebuilt. Every output is attempted, so none is left prepared for
  /// the old format: one that fails stays off until the next start(). Returns the failures,
  /// joined. Nothing may deliver frames in the old format once this runs.
  /// The caller must hold every output's mutex.
  Result<NoneType, std::string> reprepareOutputs(const StreamFormat &format);

  /// The caller must hold callback_.mutex. A callback already prepared is flushed and torn
  /// down first.
  Result<NoneType, std::string> prepareCallback(const StreamFormat &format);
  Result<NoneType, std::string> reprepareFileWriter(const StreamFormat &format);
  Result<NoneType, std::string> reprepareAdapterNode(const StreamFormat &format);

  /// Payload of @p handle, or nullptr when not connected. Valid exactly as long as the handle
  /// is held, so callers keep the handle (or adapter_.mutex) for the pointer's lifetime.
  static RecorderAdapterNode *adapterNodeOf(
      const std::shared_ptr<utils::graph::NodeHandle> &handle);

  /// Stops the audio thread from touching the side effects before they are closed.
  /// The caller must hold every output's mutex.
  DetachedSideEffects detachSideEffects();

  /// Must run with no recorder mutex held: closing the writer joins its worker thread. The
  /// file URIs are collected only after that join, once a rotation in flight has reported its file.
  Result<FileInfo, std::string> finalizeSideEffects(DetachedSideEffects sideEffects);

  void reportIfFailed(const Result<NoneType, std::string> &result);

  // ponytail: one mutex serializes every control path (the JS lane, Android's notification
  // receiver and input router, the Oboe error thread, iOS engine notifications). It goes away
  // when all of them post to a single control lane instead.
  std::mutex controlMutex_;

  std::shared_ptr<IRecorderInput> input_;
  std::shared_ptr<RecorderInputGate> inputGate_;

  SessionState session_;

  OutputSlot<AudioFileWriter> file_;
  OutputSlot<AudioRecorderCallback> callback_;
  /// Its target carries a RecorderAdapterNode; see adapterNodeOf().
  OutputSlot<utils::graph::NodeHandle> adapter_;

  /// What the next session's file is written with.
  std::shared_ptr<AudioFileProperties> fileProperties_ = nullptr;
  std::atomic<uint64_t> errorCallbackId_{0};

  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  const PlatformFileBackend &fileBackend_;
  IRecordingSessionRegistry *const sessionRegistry_;
};

} // namespace audioapi
