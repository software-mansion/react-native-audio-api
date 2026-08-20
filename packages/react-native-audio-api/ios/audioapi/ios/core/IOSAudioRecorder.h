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
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Macros.h>
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
  explicit IOSAudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const AudioRecorderOptions &options = {});
  ~IOSAudioRecorder() override;

  DELETE_COPY_AND_MOVE(IOSAudioRecorder);

  Result<NoneType, std::string> start(const std::string &fileNameOverride = "") override;
  StopResult stop() override;

  void pause() override;
  void resume() override;

  bool isRecording() const override;
  bool isPaused() const override;
  bool isIdle() const override;

  [[nodiscard]] double getInputLatency() const override;

 protected:
  /// The engine resolves the input format asynchronously, so it is read fresh on every call:
  /// between a route change and the engine settling there is no usable format at all.
  [[nodiscard]] Result<StreamFormat, std::string> resolveStreamFormat() const override;

  NativeAudioRecorder *nativeRecorder_;

 private:
  Result<NoneType, std::string> reprepareForLiveInput();
  void handleInputConfigurationChange();
  Result<NoneType, std::string> reprepareFileWriter(const StreamFormat &format);
  Result<NoneType, std::string> reprepareCallback(const StreamFormat &format);

  /// Channel count the recorder was configured with; the audio thread drops any buffer
  /// whose layout stops matching it (e.g. after a route change).
  int32_t inputChannelCount_{0};

  /// Holds the mic's planar input repacked as interleaved float32 for every consumer.
  /// Sized on the JS thread under fileWriterMutex_; never resized from the audio thread.
  std::vector<float> interleaveScratch_;
};

} // namespace audioapi
