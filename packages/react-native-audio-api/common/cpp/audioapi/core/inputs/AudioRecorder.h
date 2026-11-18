#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecorderOutputs.h>
#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/core/inputs/RecordingSessionManager.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/AudioEvent.h>
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

class AudioFileProperties;
class IAudioEventHandlerRegistry;
class RecorderInputGate;
class RecorderOutput;
struct PlatformFileBackend;
template <AudioEvent Event>
class EventCaller;

/// @brief A microphone recorder session, shared by every platform: when capture runs, which
/// format it delivers, and when its outputs (RecorderOutputs) are prepared and fed. The
/// platform's part is the capture stream behind IRecorderInput.
class AudioRecorder final : public std::enable_shared_from_this<AudioRecorder> {
 public:
  /// Reports its sessions to @p sessionManager (RecordingSessionManager::global() for the app's
  /// recorders). Writes files through @p fileBackend, which must outlive the recorder; nullptr
  /// selects the platform's (osFileBackend()).
  AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      std::shared_ptr<IRecorderInput> input,
      std::shared_ptr<RecordingSessionManager> sessionManager,
      const PlatformFileBackend *fileBackend = nullptr);

  DELETE_COPY_AND_MOVE(AudioRecorder);
  ~AudioRecorder();

  Result<NoneType, std::string> start();
  /// With @p openedGeneration, only while the input is still at that opening: once a later
  /// reroute or session has moved past it, that session is left alone and this fails.
  Result<FileInfo, std::string> stop(std::optional<uint64_t> openedGeneration = std::nullopt);
  Result<NoneType, std::string> resume();
  void pause();

  /// @brief Reopens capture on the input the platform selects now: after a route change
  /// (Android's AudioInputRouter) or a disconnected stream. A different stream format re-points the
  /// outputs without splitting the file; an output that cannot follow it is turned off and
  /// reported, and the session goes on. An input that cannot be reopened ends the session.
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

  /// Fed by every session from now on; mid-session, opened for the running one at once. Fails,
  /// adding nothing, when it cannot be opened or is added already.
  Result<NoneType, std::string> addOutput(const std::shared_ptr<RecorderOutput> &output);
  /// Closed at once if a session had opened it.
  void removeOutput(const std::shared_ptr<RecorderOutput> &output);

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();

  [[nodiscard]] double getCurrentDuration() const;

  [[nodiscard]] bool isIdle() const;
  [[nodiscard]] bool isRecording() const;
  [[nodiscard]] bool isPaused() const;
  [[nodiscard]] RecorderState getState() const;

  [[nodiscard]] double getInputLatency() const;

 private:
  /// What the session's input is doing. What holds in each phase:
  ///
  /// | Phase     | input                                       | outputs                          |
  /// |-----------|---------------------------------------------|----------------------------------|
  /// | Idle      | closed                                      | not prepared                     |
  /// | Starting  | closed, route on its way                    | not prepared                     |
  /// | Open      | open; started unless SessionState::isPaused | prepared for SessionData::format |
  /// | Rerouting | closed, route on its way                    | prepared for the old format      |
  ///
  /// Starting already holds the session's claim, so only the start that won it acquires a route.
  enum class Phase : uint8_t { Idle, Starting, Open, Rerouting };

  /// The session's whole state, read and written as one value so no reader sees a phase with the
  /// pause of another moment.
  struct SessionState {
    Phase phase = Phase::Idle;
    bool isPaused = false;

    [[nodiscard]] bool feedsOutputs() const {
      return phase == Phase::Open && !isPaused;
    }
  };
  static_assert(std::atomic<SessionState>::is_always_lock_free);

  struct SessionData {
    std::atomic<SessionState> state;

    /// The session manager's id for the current session; set by start().
    uint64_t id = 0;
    /// Bumped every time the input is closed, and by a reroute while starting, so it identifies one
    /// opening of the input, or the route a start waits for.
    uint64_t inputGeneration = 0;
    std::optional<StreamFormat> format;
    /// Written only by the audio thread, through the armed gate.
    std::atomic<double> lastBufferSeconds{0.0};
    /// Of the frames delivered while recording, each counted at the sample rate it arrived in.
    std::atomic<double> recordedSeconds{0.0};
  };

  /// Audio thread, through the armed gate only. @p channels holds @p channelCount pointers, each to
  /// numFrames float32 samples, valid only for the call.
  void onInputFrames(const float *const *channels, int channelCount, int numFrames);

  [[nodiscard]] SessionState getSessionState() const;

  /// The only place the session's state changes: arms the gate exactly when @p next feeds the
  /// outputs, and disarms it otherwise. The caller holds controlMutex_.
  void setSessionState(SessionState next);

  /// Has the outputs follow @p format when it differs from the one they are prepared for
  /// (RecorderOutputs::reprepare()), with the gate disarmed meanwhile. Leaves the gate as the
  /// state requires: armed again only when it feeds the outputs. The caller holds controlMutex_.
  Result<NoneType, std::string> repointOutputsAt(const StreamFormat &format);

  /// The step start() and reopenInput() share once the route is in and the session still waits
  /// for it: opens the input on @p route, starts it unless the session is paused, has the outputs
  /// follow the format it delivers and enters Open. A session's first opening prepares the
  /// outputs, and a failure there fails the call; a later one re-points them, and an output that
  /// cannot follow is reported while the session goes on. Leaves ending the session on failure to
  /// the caller. The caller holds controlMutex_.
  Result<NoneType, std::string> openInput(RouteLease route, InputOpenPurpose purpose);

  /// rerouteInput(), leaving a session whose input could not be reopened as it is. Sets
  /// @p reopenedGeneration to the opening it tried, which a failure belongs to.
  Result<NoneType, std::string> reopenInput(
      std::optional<uint64_t> openedGeneration,
      uint64_t &reopenedGeneration);

  /// Ends the session from any state: closes the input, releases the session's claim and the
  /// listed outputs, and takes the file away from the audio thread; RecorderOutputs::finalize()
  /// on the result is the caller's, with no mutex held. The caller holds controlMutex_.
  RecorderOutputs::Detached endSession();

  /// Delivers @p message to the JS error callback, if one is registered. Any thread.
  void reportError(const std::string &message);

  std::mutex controlMutex_;
  SessionData session_;
  std::shared_ptr<IRecorderInput> input_;
  std::shared_ptr<RecorderInputGate> inputGate_;
  std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent_;
  RecorderOutputs outputs_;
  std::shared_ptr<RecordingSessionManager> sessionManager_;
};

} // namespace audioapi
