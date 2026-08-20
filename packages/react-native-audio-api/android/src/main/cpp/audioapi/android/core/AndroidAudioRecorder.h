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
  StopResult stop() override;

  void pause() override;
  void resume() override;
  bool isRecording() const override;
  bool isPaused() const override;
  bool isIdle() const override;

  [[nodiscard]] double getInputLatency() const override;

  oboe::DataCallbackResult
  onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;
  void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

 protected:
  /// Oboe resolves the format once the stream is open, so the cached values stay valid for
  /// the whole session; a disconnect closes the stream and refreshes them on reopen.
  [[nodiscard]] Result<StreamFormat, std::string> resolveStreamFormat() const override;

 private:
  Result<NoneType, std::string> openAudioStream();

  std::string inputPreset_;
  int32_t streamChannelCount_{0};
  int32_t streamMaxBufferSizeInFrames_{0};

  std::shared_ptr<oboe::AudioStream> mStream_;
};

} // namespace audioapi
