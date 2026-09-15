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
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
  /// Whether this recorder is counted among AudioInputSelection's running
  /// captures. Guarded by streamMutex_.
  bool countedAsRunningCapture_{false};
  Result<NoneType, std::string> openAudioStream();
  /// @brief Counts this recorder among AudioInputSelection's running captures
  /// (`true`) or removes it (`false`). Idempotent.
  ///
  /// The claim must be taken before openAudioStream() reads the selection and
  /// held until the stream is running.
  ///
  /// Takes streamMutex_, which is recursive, so callers may already hold it.
  void setRunningCapture(bool running);
  std::shared_ptr<AudioFileWriter> createFileWriter(
      const std::shared_ptr<AudioFileProperties> &props);
  Result<NoneType, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties,
      const std::string &fileNameOverride = "");
};

} // namespace audioapi
