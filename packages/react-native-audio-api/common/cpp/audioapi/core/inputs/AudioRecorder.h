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

/// Platform-independent half of a microphone recorder. Subclasses own the platform input
/// stream and its lifecycle; what happens to the recorded frames afterwards -- the file
/// writer, the JS callback and the adapter node -- is configured and torn down here, so the
/// platform recorders only differ where the platforms actually differ.
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

  virtual Result<NoneType, std::string> start(const std::string &fileNameOverride) = 0;
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
  /// Format the platform input stream is currently running at.
  struct StreamFormat {
    float sampleRate = 0.0F;
    int32_t channelCount = 0;
    int32_t maxFramesPerBuffer = 0;
  };

  /// Outputs detached from a session, still holding data they have to flush. Closing them can
  /// block, so it happens in finalizeOutputs() once the caller has released the mutexes.
  struct DetachedOutputs {
    std::shared_ptr<AudioFileWriter> fileWriter;
    std::shared_ptr<AudioRecorderCallback> dataCallback;
    std::shared_ptr<utils::graph::NodeHandle> adapterNodeHandle;
    std::vector<std::string> fileUris;
  };

  /// Audio thread. Fans one normalized buffer out to the file writer, the JS callback and
  /// the adapter node. @p interleavedFrames holds numFrames * channelCount float32 samples in
  /// channel-interleaved order and is valid only for the duration of the call. Every branch
  /// takes its mutex with tryLock and drops the buffer rather than block the audio thread.
  void onAudioFrames(const float *interleavedFrames, int numFrames);

  /// Reads the format the platform input is running at. Called on the JS thread whenever an
  /// output has to be (re)prepared; fails while the input is unavailable, which on iOS happens
  /// between a route change and the engine resolving the replacement format.
  [[nodiscard]] virtual Result<StreamFormat, std::string> resolveStreamFormat() const = 0;

  /// Builds the writer for a single output file. Rotation wraps these rather than being one.
  std::shared_ptr<AudioFileWriter> createFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties);

  /// Opens the output file for the live input format and publishes the writer to the audio
  /// thread. The caller must hold fileWriterMutex_.
  Result<NoneType, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties,
      const std::string &fileNameOverride = "");

  /// Sizes the adapter node and the deinterleaving scratch for @p format.
  /// The caller must hold adapterNodeMutex_.
  void prepareAdapterNode(const StreamFormat &format);

  /// Marks every output unconfigured and hands its resources over, so the audio thread stops
  /// touching them before they are closed. The caller must hold callbackMutex_,
  /// fileWriterMutex_ and adapterNodeMutex_.
  DetachedOutputs detachOutputs();

  /// Flushes and releases what detachOutputs() handed over. Must run with no recorder mutex
  /// held: closing an encoder waits for its worker thread to drain.
  static StopResult finalizeOutputs(DetachedOutputs &&outputs);

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
  /// Every file written during the current session, in the order they were opened; a rotating
  /// writer appends one per segment.
  std::vector<std::string> recordingSegmentPaths_;
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
  /// Sample rate of the live input stream, published for readers off the JS thread.
  std::atomic<float> streamSampleRate_{0.0F};
};

} // namespace audioapi
