#pragma once

#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace audioapi {

class AudioFileWriter;
class RecorderAdapterNode;
class AudioFileProperties;
class AudioRecorderCallback;

struct AudioReadyCallbackMetadata {
  float sampleRate = 0.0f;
  size_t bufferLength = 0;
  int channelCount = 0;
  uint64_t callbackId = 0;
};

class AudioRecorder {
 public:
  using StopResult = Result<std::tuple<std::vector<std::string>, double, double>, std::string>;

  enum class RecorderState : uint8_t { Idle = 0, Recording, Paused };
  explicit AudioRecorder(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : audioEventHandlerRegistry_(audioEventHandlerRegistry),
        errorEvent_(audioEventHandlerRegistry) {}
  AudioRecorder(const AudioRecorder &) = delete;
  AudioRecorder(AudioRecorder &&) = delete;
  AudioRecorder &operator=(const AudioRecorder &) = delete;
  AudioRecorder &operator=(AudioRecorder &&) = delete;
  virtual ~AudioRecorder() = default;

  virtual Result<NoneType, std::string> start(const std::string &fileNameOverride) = 0;
  virtual StopResult stop() = 0;

  virtual Result<NoneType, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) = 0;

  /// @brief Detaches and closes the active file writer. The actual close runs
  /// outside the recorder lock. Shared by both platforms.
  void disableFileOutput();

  virtual void pause() = 0;
  virtual void resume() = 0;

  virtual void connect(const std::shared_ptr<RecorderAdapterNode> &node) = 0;

  /// @brief Tears down the connected adapter node. Shared by both platforms;
  /// platform-specific scratch state is released via clearAdapterScratchState().
  void disconnect();

  virtual Result<NoneType, std::string> setOnAudioReadyCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId) = 0;

  /// @brief Clears the audio-ready callback and its enabled/configured flags.
  /// Shared by both platforms.
  void clearOnAudioReadyCallback();

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();
  void assignOnErrorCallbackId(uint64_t callbackId);

  virtual double getCurrentDuration() const;

  bool usesCallback() const;
  bool usesFileOutput() const;
  bool isConnected() const;

  virtual bool isRecording() const = 0;
  virtual bool isPaused() const = 0;

  /// @brief Whether the recorder is idle (not recording or paused). Shared by
  /// both platforms.
  bool isIdle() const;

 protected:
  bool wantsCallback() const;
  bool wantsFileOutput() const;
  bool wantsConnection() const;
  [[nodiscard]] EventCaller<AudioEvent::RECORDER_ERROR> &errorEvent() noexcept {
    return errorEvent_;
  }
  [[nodiscard]] const EventCaller<AudioEvent::RECORDER_ERROR> &errorEvent() const noexcept {
    return errorEvent_;
  }

  /// @brief Hook for releasing platform-specific scratch state tied to a
  /// connected adapter node (e.g. Android's deinterleaving buffer). Called from
  /// disconnect() while holding adapterNodeMutex_. Default no-op.
  virtual void clearAdapterScratchState() {}

  /// @brief Builds the list of `file://` output URIs from the recorded segment
  /// paths, falling back to the single file path when no segments were tracked.
  /// Shared by the platform stop() implementations.
  static std::vector<std::string> buildOutputPaths(
      const std::vector<std::string> &segmentPaths,
      const std::string &filePath);

  /// @brief Closes the detached file writer and tears down the callback / adapter
  /// node. Must be called without holding the recorder locks.
  static StopResult finalizeStoppedOutputs(
      std::shared_ptr<AudioFileWriter> &fileWriter,
      std::shared_ptr<AudioRecorderCallback> &dataCallback,
      std::shared_ptr<RecorderAdapterNode> &adapterNode,
      std::vector<std::string> outputPaths);

  std::atomic<RecorderState> state_{RecorderState::Idle};

  std::atomic<bool> isConnected_{false};
  std::atomic<bool> fileOutputEnabled_{false};
  std::atomic<bool> callbackOutputEnabled_{false};
  std::atomic<bool> connectedConfigured_{false};
  std::atomic<bool> fileOutputConfigured_{false};
  std::atomic<bool> callbackOutputConfigured_{false};

  std::mutex callbackMutex_;
  mutable std::mutex fileWriterMutex_;
  std::mutex errorCallbackMutex_;
  mutable std::mutex adapterNodeMutex_;

  AudioReadyCallbackMetadata dataCallbackMetadata_;
  std::atomic<uint64_t> errorCallbackId_{0};

  std::string filePath_;
  std::shared_ptr<AudioFileWriter> fileWriter_ = nullptr;
  std::shared_ptr<RecorderAdapterNode> adapterNode_ = nullptr;
  std::shared_ptr<AudioRecorderCallback> dataCallback_ = nullptr;
  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;
  EventCaller<AudioEvent::RECORDER_ERROR> errorEvent_;
  std::shared_ptr<AudioFileProperties> fileProperties_ = nullptr;
};

} // namespace audioapi
