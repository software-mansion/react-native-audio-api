#pragma once

#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/libs/miniaudio/miniaudio.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/SlotFreeList.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class AudioEventHandlerRegistry;

struct CallbackData {
  size_t slot = std::numeric_limits<size_t>::max();
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

  Result<NoneType, std::string>
  prepare(float streamSampleRate, int streamChannelCount, size_t maxInputBufferLength);
  void cleanup() final;

  void receiveAudioData(void *data, int numFrames);

 protected:
  float streamSampleRate_{0.0};
  int streamChannelCount_{0};
  size_t maxInputBufferLength_{0};

  void *processingBuffer_{nullptr};
  ma_uint64 processingBufferLength_{0};
  std::unique_ptr<ma_data_converter> converter_{nullptr};

  std::shared_ptr<AudioBuffer> deinterleavingBuffer_;

  void deinterleaveAndPushAudioData(void *data, int numFrames);

 private:
  using FreeList = slots::SlotFreeList<RECORDER_CALLBACK_POOL_SIZE>;

  std::unique_ptr<std::byte[]> inputBufferPool_;
  size_t inputBufferBytesPerSlot_{0};
  std::vector<void *> inputBuffers_;
  std::unique_ptr<FreeList> freeSlots_;

  // delay initialization of offloader until prepare is called
  std::unique_ptr<task_offloader::TaskOffloader<
      CallbackData,
      RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY,
      RECORDER_CALLBACK_SPSC_WAIT_STRATEGY>>
      offloader_;
  void taskOffloaderFunction(CallbackData data);
};

} // namespace audioapi
