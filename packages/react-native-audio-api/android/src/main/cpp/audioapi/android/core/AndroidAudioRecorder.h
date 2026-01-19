#pragma once

#include <audioapi/core/inputs/AudioRecorder.h>
#include <oboe/Oboe.h>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <mutex>
#include <audioapi/utils/Result.hpp>
#include <audioapi/android/core/NativeAudioRecorder.hpp>

namespace audioapi {

class AudioBus;
class AudioArray;
class CircularAudioArray;
class AudioFileProperties;
class AndroidRecorderCallback;
class AndroidFileWriterBackend;
class AudioEventHandlerRegistry;

class AndroidAudioRecorder : public oboe::AudioStreamCallback, public AudioRecorder {
 public:
  explicit AndroidAudioRecorder(const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~AndroidAudioRecorder() override;
  void cleanup();

  Result<std::string, std::string> start(const std::string &fileNameOverride) override;
  Result<std::tuple<std::string, double, double>, std::string> stop() override;

  Result<std::string, std::string> enableFileOutput(std::shared_ptr<AudioFileProperties> properties) override;
  void disableFileOutput() override;

  void pause() override;
  void resume() override;
  bool isRecording() const override;
  bool isPaused() const override;
  bool isIdle() const override;

  Result<NoneType, std::string> setOnAudioReadyCallback(float sampleRate, size_t bufferLength, int channelCount, uint64_t callbackId)
      override;
  void clearOnAudioReadyCallback() override;

  void connect(const std::shared_ptr<RecorderAdapterNode> &node) override;
  void disconnect() override;

  oboe::DataCallbackResult onAudioReady(
          oboe::AudioStream *oboeStream,
          void *audioData,
          int32_t numFrames) override;
  void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

 private:
  std::shared_ptr<AudioArray> deinterleavingBuffer_;

  float streamSampleRate_;
  int32_t streamChannelCount_;
  int32_t streamMaxBufferSizeInFrames_;

  facebook::jni::global_ref<NativeAudioRecorder> nativeAudioRecorder_;

  std::shared_ptr<oboe::AudioStream> mStream_;
  Result<NoneType, std::string> openAudioStream();
};

} // namespace audioapi
