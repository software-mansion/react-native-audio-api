#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <tuple>
#include <string>
#include <memory>
#include <audioapi/utils/Result.hpp>

struct WriterData {
  void *data;
  int numFrames;
};

namespace audioapi {

class AudioFileProperties;

class AndroidFileWriterBackend : public AudioFileWriter {
 public:
  explicit AndroidFileWriterBackend(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties);

  virtual OpenFileResult openFile(float streamSampleRate, int32_t streamChannelCount, int32_t streamMaxBufferSize, const std::string &fileNameOverride) = 0;
  void writeAudioData(void *data, int numFrames);

  std::string getFilePath() const override { return filePath_; }
  double getCurrentDuration() const override { return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / streamSampleRate_; }

 protected:
  float streamSampleRate_{0};
  int32_t streamChannelCount_{0};
  int32_t streamMaxBufferSize_{0};
  std::string filePath_;

  // delay initialization of offloader until prepare is called
  std::unique_ptr<task_offloader::TaskOffloader<WriterData, FILE_WRITER_SPSC_OVERFLOW_STRATEGY, FILE_WRITER_SPSC_WAIT_STRATEGY>> offloader_;
  virtual void taskOffloaderFunction(WriterData data) = 0;
};

} // namespace audioapi
