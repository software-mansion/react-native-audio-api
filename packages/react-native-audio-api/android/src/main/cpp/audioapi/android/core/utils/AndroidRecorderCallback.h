#pragma once


#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <memory>
#include <vector>
#include <string>

namespace audioapi {

class AudioBus;
class AudioArray;
class CircularAudioArray;
class AudioEventHandlerRegistry;

struct CallbackData {
    void *data;
    int numFrames;
};

class AndroidRecorderCallback : public AudioRecorderCallback {
 public:
  AndroidRecorderCallback(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  ~AndroidRecorderCallback() override;

  Result<NoneType, std::string> prepare(float streamSampleRate, int streamChannelCount, size_t maxInputBufferLength);
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

 private:
  channels::spsc::Sender<CallbackData, AudioRecorderCallback::RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY, AudioRecorderCallback::RECORDER_CALLBACK_SPSC_WAIT_STRATEGY> sender_;
  channels::spsc::Receiver<CallbackData, AudioRecorderCallback::RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY, AudioRecorderCallback::RECORDER_CALLBACK_SPSC_WAIT_STRATEGY> receiver_;
  void callbackThreadHandler();
};

} // namespace audioapi
