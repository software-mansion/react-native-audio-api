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
#include <audioapi/utils/Result.hpp>

#include <mutex>

namespace audioapi {

class FileWriter;
class RecorderCallback;
class RecorderAdapterNode;
class AudioFileProperties;
class AudioEventHandlerRegistry;

class IOSAudioRecorder : public AudioRecorder {
 public:
  IOSAudioRecorder(const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~IOSAudioRecorder() override;

  Result<std::string, std::string> start(const std::string &fileNameOverride = "") override;
  Result<std::tuple<std::string, double, double>, std::string> stop() override;

  Result<std::string, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) override;
  void disableFileOutput() override;

  void connect(const std::shared_ptr<RecorderAdapterNode> &node) override;
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

 protected:
  NativeAudioRecorder *nativeRecorder_;
};

} // namespace audioapi
