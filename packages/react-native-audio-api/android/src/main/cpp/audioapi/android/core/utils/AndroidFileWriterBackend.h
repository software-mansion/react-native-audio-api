#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <tuple>
#include <string>
#include <memory>
#include <audioapi/utils/Result.hpp>

namespace audioapi {

class AudioFileProperties;

class AndroidFileWriterBackend : public AudioFileWriter {
 public:
  explicit AndroidFileWriterBackend(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
      : AudioFileWriter(audioEventHandlerRegistry, fileProperties) {}

  virtual OpenFileResult openFile(float streamSampleRate, int32_t streamChannelCount, int32_t streamMaxBufferSize, const std::string &fileNameOverride) = 0;
  virtual bool writeAudioData(void *data, int numFrames) = 0;

  std::string getFilePath() const override { return filePath_; }
  double getCurrentDuration() const override { return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / streamSampleRate_; }

 protected:
  float streamSampleRate_{0};
  int32_t streamChannelCount_{0};
  int32_t streamMaxBufferSize_{0};
  std::string filePath_{""};
};

} // namespace audioapi
