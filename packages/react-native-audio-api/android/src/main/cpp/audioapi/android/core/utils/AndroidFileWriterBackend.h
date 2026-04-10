#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <memory>
#include <string>

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

  void writeAudioData(AudioDataType data, int numFrames) override;

  std::string getFilePath() const override {
    return filePath_;
  }
  double getCurrentDuration() const override {
    return static_cast<double>(framesWritten_.load(std::memory_order_acquire)) / streamSampleRate_;
  }
  size_t getFileSizeBytes() const override {
    return 0;
  }

  virtual OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t streamMaxBufferSize,
      const std::string &fileNameOverride) = 0;
  virtual void taskOffloaderFunction(WriterData data) = 0;

 protected:
  float streamSampleRate_;
  int32_t streamChannelCount_;
  int32_t streamMaxBufferSize_;
  std::string filePath_;

  // delay initialization of offloader until prepare is called
  std::unique_ptr<task_offloader::TaskOffloader<
      WriterData,
      FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      FILE_WRITER_SPSC_WAIT_STRATEGY>>
      offloader_;
};

} // namespace audioapi
