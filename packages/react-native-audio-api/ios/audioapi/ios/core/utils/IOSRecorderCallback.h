#pragma once

#ifndef __OBJC__ // when compiled as C++
typedef struct objc_object AVAudioFormat;
typedef struct objc_object AudioBufferList;
typedef struct objc_object AVAudioConverter;
#endif

#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <audioapi/utils/Result.hpp>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBuffer;
class CircularAudioArray;
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
};

} // namespace audioapi
