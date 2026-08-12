#pragma once

#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

namespace audioapi {

class AudioFileWriter;
class AudioFileProperties;
class AudioRecorderCallback;
class IAudioEventHandlerRegistry;

class AudioRecorder {
 public:
  enum class RecorderState : uint8_t { Idle = 0, Recording, Paused };
  explicit AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : audioEventHandlerRegistry_(audioEventHandlerRegistry) {}
  AudioRecorder(const AudioRecorder &) = delete;
  AudioRecorder(AudioRecorder &&) = delete;
  AudioRecorder &operator=(const AudioRecorder &) = delete;
  AudioRecorder &operator=(AudioRecorder &&) = delete;
  virtual ~AudioRecorder() = default;

  virtual Result<NoneType, std::string> start(const std::string &fileNameOverride) = 0;
  virtual Result<std::tuple<std::vector<std::string>, double, double>, std::string> stop() = 0;

  virtual Result<NoneType, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) = 0;
  virtual void disableFileOutput() = 0;

  virtual void pause() = 0;
  virtual void resume() = 0;

  virtual void connect(const std::shared_ptr<utils::graph::NodeHandle> &node) = 0;
  virtual void disconnect() = 0;

  virtual Result<NoneType, std::string> setOnAudioReadyCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId) = 0;
  virtual void clearOnAudioReadyCallback() = 0;

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();

  virtual double getCurrentDuration() const;

  bool usesCallback() const;
  bool usesFileOutput() const;
  bool isConnected() const;

  virtual bool isRecording() const = 0;
  virtual bool isPaused() const = 0;
  virtual bool isIdle() const = 0;

  [[nodiscard]] virtual double getInputLatency() const = 0;

 protected:
  /// Audio thread. Fans one normalized buffer out to the file writer, the JS callback and
  /// the adapter node. @p interleavedFrames holds numFrames * channelCount float32 samples in
  /// channel-interleaved order and is valid only for the duration of the call. Every branch
  /// takes its mutex with tryLock and drops the buffer rather than block the audio thread.
  void onAudioFrames(const float *interleavedFrames, int numFrames);

  bool wantsCallback() const;
  bool wantsFileOutput() const;
  bool wantsConnection() const;

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
  mutable std::recursive_mutex streamMutex_;

  std::atomic<uint64_t> errorCallbackId_{0};

  std::string filePath_;
  std::shared_ptr<AudioFileWriter> fileWriter_ = nullptr;
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle_ = nullptr;
  std::shared_ptr<AudioRecorderCallback> dataCallback_ = nullptr;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  std::shared_ptr<AudioFileProperties> fileProperties_ = nullptr;
  /// Scratch for the adapter-node branch, which needs planar channels. Allocated on the JS
  /// thread when a node connects; snapshotted by the audio thread so it cannot be freed mid-use.
  std::shared_ptr<AudioBuffer> deinterleavingBuffer_ = nullptr;
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
};

} // namespace audioapi
