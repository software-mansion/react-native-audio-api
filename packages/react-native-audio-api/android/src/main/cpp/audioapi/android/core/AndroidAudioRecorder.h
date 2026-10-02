#pragma once

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <oboe/Oboe.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace audioapi {

class AudioFileProperties;
class AndroidRecorderCallback;
class AndroidFileWriterBackend;
class IAudioEventHandlerRegistry;

class AndroidAudioRecorder : public oboe::AudioStreamCallback,
                             public AudioRecorder,
                             public std::enable_shared_from_this<AndroidAudioRecorder> {
 public:
  explicit AndroidAudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      AudioRecorderOptions options = {});
  ~AndroidAudioRecorder() override;
  void cleanup();

  DELETE_COPY_AND_MOVE(AndroidAudioRecorder);

  Result<NoneType, std::string> start(const std::string &fileNameOverride) override;
  Result<std::tuple<std::vector<std::string>, double, double>, std::string> stop() override;

  Result<NoneType, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) override;
  void disableFileOutput() override;

  void pause() override;
  void resume() override;
  /// @brief Reopens the capture stream on the input MediaSessionManager currently selects.
  /// A stream format that differs from the previous one starts a new file segment and
  /// re-prepares the callback and the adapter node. Does nothing while idle.
  Result<NoneType, std::string> rerouteInput() override;
  bool isRecording() const override;
  bool isPaused() const override;
  bool isIdle() const override;

  Result<NoneType, std::string> setOnAudioReadyCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId) override;
  void clearOnAudioReadyCallback() override;

  void connect(const std::shared_ptr<utils::graph::NodeHandle> &node) override;
  void disconnect() override;

  [[nodiscard]] double getInputLatency() const override;
  /// Counts the segments already closed by a format change, not only the open one.
  double getCurrentDuration() const override;

  oboe::DataCallbackResult
  onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;
  void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

 private:
  std::shared_ptr<AudioBuffer> deinterleavingBuffer_;

  std::string inputPreset_;
  std::atomic<float> streamSampleRate_;
  int32_t streamChannelCount_;
  int32_t streamMaxBufferSizeInFrames_;
  /// The device mStream_ was opened for, guarded by streamMutex_.
  int32_t streamDeviceId_;

  std::shared_ptr<oboe::AudioStream> mStream_;
  std::vector<std::string> recordingSegmentPaths_;
  double closedSegmentsSizeMb_{0.0};
  double closedSegmentsDuration_{0.0};
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  /// Whether this recorder holds a route from NativeInputRouting::acquireInputRoute.
  /// Guarded by streamMutex_.
  bool holdsInputRoute_{false};
  /// The device of that route. Guarded by streamMutex_.
  int32_t captureDeviceId_{0};
  Result<NoneType, std::string> openAudioStream();
  void closeAudioStream();

  struct StreamFormat {
    float sampleRate;
    int32_t channelCount;
    int32_t maxBufferSizeInFrames;

    bool operator==(const StreamFormat &) const = default;
  };
  [[nodiscard]] StreamFormat streamFormat() const;

  /// @brief Closes the capture stream, opens a new one on the currently selected input and
  /// carries the recording or paused session over to it.
  ///
  /// Call with none of the recorder's mutexes held.
  Result<NoneType, std::string> restartCapture();
  /// @brief Opens the stream on the adopted route, re-prepares the outputs when its format
  /// differs from @p previousFormat and starts it when @p stateToRestore is Recording.
  ///
  /// The caller must hold callbackMutex_, fileWriterMutex_, adapterNodeMutex_ and streamMutex_.
  Result<NoneType, std::string> reopenAudioStream(
      const StreamFormat &previousFormat,
      RecorderState stateToRestore);
  void endSessionAfterFailedRestart(const std::string &reason);
  /// The caller must hold the same mutexes as reopenAudioStream().
  Result<NoneType, std::string> reprepareOutputs();
  /// The caller must hold callbackMutex_.
  Result<NoneType, std::string> prepareCallback();
  /// The caller must hold adapterNodeMutex_.
  void prepareAdapterNode();
  /// File name for the next segment of a single-file recording, derived from the first
  /// segment so that two segments opened within the same second cannot collide.
  [[nodiscard]] std::string nextSegmentFileName() const;
  void reportError(const std::string &message);
  /// @brief Records a route the caller took with NativeInputRouting::acquireInputRoute(), which
  /// openAudioStream() then opens on. acquireInputRoute() itself can block for seconds, so it is
  /// called with none of the recorder's mutexes held.
  void adoptInputRoute(std::optional<int32_t> routedDeviceId);
  /// @brief Hands the route back; does nothing when none is held.
  void releaseInputRoute();
  std::shared_ptr<AudioFileWriter> createFileWriter(
      const std::shared_ptr<AudioFileProperties> &props);
  Result<NoneType, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties,
      const std::string &fileNameOverride = "");
};

} // namespace audioapi
