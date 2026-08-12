#pragma once

#ifdef __OBJC__ // when compiled as Objective-C
#import <NativeAudioRecorder.h>
#else
typedef struct objc_object NSURL;
typedef struct objc_object AVAudioFile;
typedef struct objc_object AudioBufferList;
typedef struct objc_object NativeAudioRecorder;
#endif // __OBJC__

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class RecorderCallback;
class RecorderAdapterNode;
class AudioFileProperties;
class IAudioEventHandlerRegistry;
class AudioFileWriter;

class IOSAudioRecorder : public AudioRecorder {
 public:
  IOSAudioRecorder(const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~IOSAudioRecorder() override;

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

  /// Channel count the recorder was configured with; the audio thread drops any buffer
  /// whose layout stops matching it (e.g. after a route change).
  int32_t inputChannelCount_{0};

  /// Holds the mic's planar input repacked as interleaved float32 for every consumer.
  /// Sized on the JS thread under fileWriterMutex_; never resized from the audio thread.
  std::vector<float> interleaveScratch_;

  std::vector<std::string> recordingSegmentPaths_;
  std::atomic<float> streamSampleRate_{0.0f};
};

} // namespace audioapi
