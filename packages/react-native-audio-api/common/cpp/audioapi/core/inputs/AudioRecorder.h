#pragma once

#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace audioapi {

class AudioFileWriter;
class AudioFileProperties;
class AudioRecorderCallback;
class IAudioEventHandlerRegistry;

/// Platform-independent half of a microphone recorder: subclasses own the platform input
/// stream; the file writer, the JS callback and the adapter node are managed here.
class AudioRecorder {
 public:
  enum class RecorderState : uint8_t { Idle = 0, Recording, Paused };

  /// Every file the session produced, its total size in MB and its total duration in seconds.
  using StopResult = Result<std::tuple<std::vector<std::string>, double, double>, std::string>;

  explicit AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : audioEventHandlerRegistry_(audioEventHandlerRegistry) {}
  DELETE_COPY_AND_MOVE(AudioRecorder);
  virtual ~AudioRecorder() = default;

  virtual Result<NoneType, std::string> start() = 0;
  virtual StopResult stop() = 0;

  Result<NoneType, std::string> enableFileOutput(std::shared_ptr<AudioFileProperties> properties);
  void disableFileOutput();

  virtual void pause() = 0;
  virtual void resume() = 0;

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

  [[nodiscard]] virtual double getInputLatency() const = 0;

 protected:
  struct StreamFormat {
    float sampleRate = 0.0F;
    int32_t channelCount = 0;
    int32_t maxFramesPerBuffer = 0;
  };

  /// Closing these can block, so it happens in finalizeSideEffects() with no mutex held.
  struct DetachedSideEffects {
    std::shared_ptr<AudioFileWriter> fileWriter;
    std::shared_ptr<AudioRecorderCallback> dataCallback;
    std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
    std::vector<std::string> fileUris;
  };

  /// Audio thread. @p interleavedFrames holds numFrames * channelCount interleaved float32
  /// samples, valid only for the call. Every consumer tryLocks its mutex and drops the buffer
  /// rather than block.
  void onAudioFrames(const float *interleavedFrames, int numFrames);

  /// JS thread. Fails while the input is unavailable, which on iOS happens between a route
  /// change and the engine settling on the replacement format.
  [[nodiscard]] virtual Result<StreamFormat, std::string> resolveStreamFormat() const = 0;

  /// The caller must hold fileWriterMutex_.
  Result<NoneType, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties);

  /// The caller must hold adapterNodeMutex_.
  void prepareAdapterNode(const StreamFormat &format);

  /// Stops the audio thread from touching the side effects before they are closed.
  /// The caller must hold callbackMutex_, fileWriterMutex_ and adapterNodeMutex_.
  DetachedSideEffects detachSideEffects();

  /// Must run with no recorder mutex held: closing the writer joins its worker thread. The
  /// file URIs are collected only after that join, once a rotation in flight has reported its file.
  StopResult finalizeSideEffects(DetachedSideEffects &&sideEffects);

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
  /// Guards recordingSegmentPaths_, which the writer's worker thread appends to on rotation.
  /// Its own lock because the JS thread holds fileWriterMutex_ while joining that worker.
  std::mutex segmentPathsMutex_;
  mutable std::mutex adapterNodeMutex_;
  mutable std::recursive_mutex streamMutex_;

  std::atomic<uint64_t> errorCallbackId_{0};

  std::string filePath_;
  /// Every file of the current session, in open order. Guarded by segmentPathsMutex_.
  std::vector<std::string> recordingSegmentPaths_;
  std::shared_ptr<AudioFileWriter> fileWriter_ = nullptr;
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle_ = nullptr;
  std::shared_ptr<AudioRecorderCallback> dataCallback_ = nullptr;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  std::shared_ptr<AudioFileProperties> fileProperties_ = nullptr;
  /// Allocated on the JS thread when a node connects; the audio thread copies the shared_ptr
  /// so it cannot be freed mid-use.
  std::shared_ptr<AudioBuffer> deinterleavingBuffer_ = nullptr;
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  /// Sample rate of the live input stream, published for readers off the JS thread.
  std::atomic<float> streamSampleRate_{0.0F};
};

} // namespace audioapi
