#pragma once

#include <audioapi/android/core/NativeAudioRecorder.hpp>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <oboe/Oboe.h>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioFileProperties;
class AndroidRecorderCallback;
class AndroidFileWriterBackend;
class AudioEventHandlerRegistry;

class AndroidAudioRecorder : public oboe::AudioStreamCallback,
                             public AudioRecorder,
                             public std::enable_shared_from_this<AndroidAudioRecorder> {
 public:
  explicit AndroidAudioRecorder(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~AndroidAudioRecorder() override;
  void cleanup();

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

  void connect(const std::shared_ptr<RecorderAdapterNode> &node) override;
  void disconnect() override;

  oboe::DataCallbackResult
  onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;
  void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

 private:
  std::shared_ptr<AudioBuffer> deinterleavingBuffer_;

  float streamSampleRate_;
  int32_t streamChannelCount_;
  int32_t streamMaxBufferSizeInFrames_;

  facebook::jni::global_ref<NativeAudioRecorder> nativeAudioRecorder_;

  std::shared_ptr<oboe::AudioStream> mStream_;
  std::vector<std::string> recordingSegmentPaths_;
  Result<NoneType, std::string> openAudioStream();
  std::shared_ptr<AudioFileWriter> createFileWriter(
      const std::shared_ptr<AudioFileProperties> &props);
  Result<NoneType, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties,
      const std::string &fileNameOverride = "");
};

} // namespace audioapi
