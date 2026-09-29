#pragma once

#include <audioapi/core/inputs/FileInfo.h>
#include <audioapi/core/inputs/RecorderState.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
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

/// Platform-independent half of a microphone recorder: subclasses own the platform input
/// stream; the file writer, the JS callback and the adapter node are managed here.
class AudioRecorder {
 public:
  explicit AudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : audioEventHandlerRegistry_(audioEventHandlerRegistry) {}
  DELETE_COPY_AND_MOVE(AudioRecorder);
  virtual ~AudioRecorder() = default;

  virtual Result<NoneType, std::string> start() = 0;
  virtual Result<FileInfo, std::string> stop() = 0;

  Result<NoneType, std::string> enableFileOutput(std::shared_ptr<AudioFileProperties> properties);
  void disableFileOutput();

  virtual void pause() = 0;
  virtual void resume() = 0;

  /// @p adapterNode is the payload of @p node; the handle keeps it alive while connected.
  void connect(
      const std::shared_ptr<utils::graph::NodeHandle> &node,
      RecorderAdapterNode *adapterNode);
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

  [[nodiscard]] virtual double getInputLatency() const = 0;

 protected:
  /// @brief Lifecycle of one recorder output: the file writer, the JS callback, or the adapter
  /// node. Written on the JS thread under that output's mutex; the audio thread reads it
  /// lock-free to decide whether to feed the output.
  enum class OutputState : uint8_t {
    /// Not requested.
    Disabled,
    /// Requested, but not yet prepared for the current stream format. The next start(), or an
    /// input format change, prepares it.
    Requested,
    /// Prepared for the current stream format; the audio thread may feed it.
    Active,
  };

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
    RecorderAdapterNode *adapterNode = nullptr;
    std::vector<std::string> fileUris;
  };

  /// Audio thread. @p channels holds one pointer per stream channel, each to numFrames float32
  /// samples, valid only for the call. Every consumer tryLocks its mutex and drops the buffer
  /// rather than block.
  void onAudioFrames(const float *const *channels, int numFrames);

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
  Result<FileInfo, std::string> finalizeSideEffects(DetachedSideEffects &&sideEffects);

  bool wantsCallback() const;
  bool wantsFileOutput() const;
  bool wantsConnection() const;

  /// Active becomes Requested; Disabled stays Disabled, so an output that was never requested
  /// cannot be switched on by a failed prepare.
  static void deactivate(std::atomic<OutputState> &state);

  std::atomic<RecorderState> state_{RecorderState::Idle};

  std::atomic<OutputState> fileOutputState_{OutputState::Disabled};
  std::atomic<OutputState> callbackOutputState_{OutputState::Disabled};
  std::atomic<OutputState> connectionState_{OutputState::Disabled};

  std::mutex callbackMutex_;
  mutable std::mutex fileWriterMutex_;
  std::mutex errorCallbackMutex_;
  mutable std::mutex adapterNodeMutex_;

  std::atomic<uint64_t> errorCallbackId_{0};

  std::shared_ptr<AudioFileWriter> fileWriter_ = nullptr;
  std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle_ = nullptr;
  /// Payload of adapterNodeHandle_. Valid exactly as long as that handle is held, so the two
  /// are set and cleared together under adapterNodeMutex_.
  RecorderAdapterNode *adapterNode_ = nullptr;
  std::shared_ptr<AudioRecorderCallback> dataCallback_ = nullptr;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  std::shared_ptr<AudioFileProperties> fileProperties_ = nullptr;
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  /// Sample rate of the live input stream, published for readers off the JS thread.
  std::atomic<float> streamSampleRate_{0.0F};
};

} // namespace audioapi
