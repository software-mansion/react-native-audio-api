#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/RecorderOutputs.h>
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

namespace audioapi {

class AudioFileProperties;
class IAudioEventHandlerRegistry;
class IRecorderInput;
class RecorderInputGate;
struct PlatformFileBackend;

/// @brief A microphone recorder session, shared by every platform: when capture runs, which
/// format it delivers, and when its outputs (RecorderOutputs) are prepared and fed. The
/// platform's part is the capture stream behind IRecorderInput.
class AudioRecorder final : public std::enable_shared_from_this<AudioRecorder> {
 public:
  /// Writes files through @p fileBackend, which must outlive the recorder; nullptr selects the
  /// platform's (osFileBackend()).
  AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      std::shared_ptr<IRecorderInput> input,
      const PlatformFileBackend *fileBackend = nullptr);

  DELETE_COPY_AND_MOVE(AudioRecorder);
  ~AudioRecorder();

  Result<NoneType, std::string> start();
  Result<FileInfo, std::string> stop();
  Result<NoneType, std::string> resume();
  void pause();

  /// @brief Reopens capture on the input the platform selects now: after a route change
  /// (Android's AudioInputRouter) or a disconnected stream. A different stream format re-points the
  /// outputs without splitting the file.
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

  [[nodiscard]] double getCurrentDuration() const;

  [[nodiscard]] bool isIdle() const;
  [[nodiscard]] bool isRecording() const;
  [[nodiscard]] bool isPaused() const;
  [[nodiscard]] RecorderState getState() const;

  [[nodiscard]] double getInputLatency() const;

 private:
  friend class RecorderInputGate;

  /// Where the session is. What holds in each phase:
  ///
  /// | Phase     | input                    | gate     | outputs                           |
  /// |-----------|--------------------------|----------|-----------------------------------|
  /// | Idle      | closed                   | disarmed | not prepared                      |
  /// | Recording | open, started            | armed    | prepared for SessionState::format |
  /// | Paused    | open, stopped by pause() | disarmed | prepared for SessionState::format |
  /// | Rerouting | closed, route on its way | disarmed | prepared for the old format       |
  enum class Phase : uint8_t { Idle, Recording, Paused, Rerouting };

  struct SessionState {
    std::atomic<Phase> phase{Phase::Idle};
    /// Recording or Paused: what a reroute brings the session back as.
    std::atomic<Phase> phaseAfterReroute{Phase::Recording};

    /// Bumped every time the input is closed, so it identifies one opening of the input.
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

  /// The only place the session changes phase: frames reach the outputs exactly while
  /// recording, so it arms the gate for Recording and disarms it for any other phase. The caller
  /// holds controlMutex_.
  void enterPhase(Phase phase);

  /// Has the outputs follow @p format when it differs from the one they are prepared for
  /// (RecorderOutputs::follow()), with the gate disarmed meanwhile. Leaves the gate as the
  /// phase requires: armed again only when recording. The caller holds controlMutex_.
  Result<NoneType, std::string> repointOutputsAt(const StreamFormat &format);

  /// rerouteInput(), leaving a session whose input could not be reopened as it is.
  Result<NoneType, std::string> reopenInput(std::optional<uint64_t> openedGeneration);

  /// Ends the session from any phase: closes the input and takes the outputs away from the audio
  /// thread; RecorderOutputs::finalize() on the result is the caller's, with no mutex held. The
  /// caller holds controlMutex_.
  RecorderOutputs::Detached endSession();

  /// Delivers @p message to the JS error callback, if one is registered. Any thread.
  void reportError(const std::string &message);

  std::mutex controlMutex_;
  SessionState session_;
  std::shared_ptr<IRecorderInput> input_;
  std::shared_ptr<RecorderInputGate> inputGate_;
  RecorderOutputs outputs_;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
};

} // namespace audioapi
