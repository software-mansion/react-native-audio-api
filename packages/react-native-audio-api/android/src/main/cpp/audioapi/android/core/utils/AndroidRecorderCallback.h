#pragma once

#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioBuffer;
class AudioArray;
class CircularAudioArray;
class AudioEventHandlerRegistry;

class AndroidRecorderCallback : public AudioRecorderCallback {
 public:
  AndroidRecorderCallback(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  ~AndroidRecorderCallback();

  Result<NoneType, std::string>
  prepare(float streamSampleRate, int streamChannelCount, size_t maxInputBufferLength);
  void cleanup() override;

  void receiveAudioData(void *data, int numFrames);

 protected:
  float streamSampleRate_{0.0};
  int streamChannelCount_{0};
  size_t maxInputBufferLength_{0};

  void *processingBuffer_{nullptr};
  ma_uint64 processingBufferLength_{0};
  std::unique_ptr<ma_data_converter> converter_{nullptr};

  std::shared_ptr<AudioArray> deinterleavingArray_;

  void deinterleaveAndPushAudioData(void *data, int numFrames);
};

} // namespace audioapi
