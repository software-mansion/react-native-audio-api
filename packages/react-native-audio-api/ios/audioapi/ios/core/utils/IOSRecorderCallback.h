#pragma once

#ifndef __OBJC__ // when compiled as C++
typedef struct objc_object AVAudioFormat;
typedef struct objc_object AVAudioConverter;
// AudioBufferList is forward-declared as a C struct by OwnedAudioBufferList.h.
#endif

#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/ios/core/utils/OwnedAudioBufferList.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SlotFreeList.hpp>
#include <limits>
#include <memory>
#include <vector>

struct CallbackData {
  size_t slot = std::numeric_limits<size_t>::max();
  int numFrames;
};

namespace audioapi {

class AudioEventHandlerRegistry;

class IOSRecorderCallback : public AudioRecorderCallback {
 public:
  IOSRecorderCallback(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);
  ~IOSRecorderCallback();

  Result<NoneType, std::string> prepare(AVAudioFormat *bufferFormat, size_t maxInputBufferLength);
  void cleanup() override;

  void receiveAudioData(const AudioBufferList *audioBufferList, int numFrames);

 protected:
  size_t converterInputBufferSize_;
  size_t converterOutputBufferSize_;

  AVAudioFormat *bufferFormat_;
  AVAudioFormat *callbackFormat_;
  AVAudioConverter *converter_;

  AVAudioPCMBuffer *converterInputBuffer_;
  AVAudioPCMBuffer *converterOutputBuffer_;

 private:
  using FreeList = slots::SlotFreeList<RECORDER_CALLBACK_POOL_SIZE>;

  std::vector<ios::OwnedAudioBufferListPtr> inputBufferPool_;
  size_t inputBufferBytesPerBuffer_{0};
  std::unique_ptr<FreeList> freeSlots_;

  std::unique_ptr<task_offloader::TaskOffloader<
      CallbackData,
      RECORDER_CALLBACK_SPSC_OVERFLOW_STRATEGY,
      RECORDER_CALLBACK_SPSC_WAIT_STRATEGY>>
      offloader_;
  // delay initialization of offloader until prepare is called
  void taskOffloaderFunction(CallbackData data);
};

} // namespace audioapi
