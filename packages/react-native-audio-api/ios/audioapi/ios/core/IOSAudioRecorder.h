#pragma once

#ifdef __OBJC__ // when compiled as Objective-C
#import <audioapi/ios/core/NativeAudioRecorder.h>
#else
typedef struct objc_object NSURL;
typedef struct objc_object AVAudioFile;
typedef struct objc_object AudioBufferList;
typedef struct objc_object NativeAudioRecorder;
typedef struct objc_object AVAudioFormat;
#endif // __OBJC__

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace facebook::react {
class CallInvoker;
}

namespace audioapi {

class RecorderCallback;
class RecorderAdapterNode;
class AudioFileProperties;
class AudioEventHandlerRegistry;
class AudioFileWriter;

class IOSAudioRecorder : public AudioRecorder,
                         public std::enable_shared_from_this<IOSAudioRecorder> {
 public:
  IOSAudioRecorder(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker);
  ~IOSAudioRecorder() override;

  void bindLifetime();

  Result<NoneType, std::string> start(const std::string &fileNameOverride = "") override;
  Result<std::tuple<std::vector<std::string>, double, double>, std::string> stop() override;

  Result<NoneType, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) override;
  void disableFileOutput() override;

  void connect(const std::shared_ptr<utils::graph::NodeHandle> &node) override;
  void disconnect() override;

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

  [[nodiscard]] double getInputLatency() const override;

 protected:
  NativeAudioRecorder *nativeRecorder_;

 private:
  std::shared_ptr<AudioFileWriter> createFileWriter(
      const std::shared_ptr<AudioFileProperties> &props);
  Result<std::string, std::string> setupFileWriter(
      const std::shared_ptr<AudioFileProperties> &properties,
      const std::string &fileNameOverride = "");
  Result<NoneType, std::string> reprepareForLiveInput();
  void scheduleReprepareForLiveInput();
  void handleInputConfigurationChange();
  Result<NoneType, std::string> reprepareFileWriter(
      AVAudioFormat *inputFormat,
      int maxInputBufferLength);
  Result<NoneType, std::string> reprepareCallback(
      AVAudioFormat *inputFormat,
      int maxInputBufferLength);
  void reprepareAdapter(AVAudioFormat *inputFormat, int maxInputBufferLength);
  void runSideEffects(const AudioBufferList *inputBuffer, int numFrames);

  std::weak_ptr<AudioRecorder> lifetime_;
  std::shared_ptr<facebook::react::CallInvoker> jsCallInvoker_;

  std::vector<std::string> recordingSegmentPaths_;
  std::atomic<float> streamSampleRate_{0.0f};
  /// Updated on the audio thread from each input callback `numFrames`.
  std::atomic<int32_t> lastCallbackFrameCount_{0};
};

} // namespace audioapi
