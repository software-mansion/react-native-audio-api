#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SlotFreeList.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <cstddef>
#include <limits>
#include <memory>
#include <string>
#include <vector>

struct WriterData {
  size_t slot = std::numeric_limits<size_t>::max();
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
  virtual void processWriterData(void *data, int numFrames) = 0;

 protected:
  using FreeList = slots::SlotFreeList<FILE_WRITER_POOL_SIZE>;

  float streamSampleRate_;
  int32_t streamChannelCount_;
  int32_t streamMaxBufferSize_;
  std::string filePath_;

  bool initializePreallocatedInputPool();
  void cleanupPreallocatedInputPool();

  // Lifetime tied to the preallocated pool so a writer can be reopened after closeFile().
  std::unique_ptr<task_offloader::TaskOffloader<
      WriterData,
      FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      FILE_WRITER_SPSC_WAIT_STRATEGY>>
      offloader_;

 private:
  void createOffloader();
  void runWriterTask(WriterData data);

  std::unique_ptr<std::byte[]> inputBufferPool_;
  size_t inputBufferBytesPerSlot_{0};
  std::vector<void *> inputBuffers_;
  std::unique_ptr<FreeList> freeSlots_;
};

} // namespace audioapi
