#pragma once

#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <atomic>
#include <string>
#include <memory>
#include <tuple>

namespace audioapi {

class MiniAudioFileWriter : public AndroidFileWriterBackend {
 public:
  explicit MiniAudioFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties);
  ~MiniAudioFileWriter();

  OpenFileResult openFile(float streamSampleRate, int32_t streamChannelCount, int32_t streamMaxBufferSize, const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

  bool writeAudioData(void *data, int numFrames) override;

 private:
  std::atomic<bool> isConverterRequired_{false};

  std::unique_ptr<ma_encoder> encoder_{nullptr};
  std::unique_ptr<ma_data_converter> converter_{nullptr};
  void *processingBuffer_{nullptr};
  ma_uint64 processingBufferLength_{0};

  ma_result initializeConverterIfNeeded();
  ma_result initializeEncoder(const std::string &fileNameOverride);
  ma_uint64 convertBuffer(void *data, int numFrames);

  bool isConverterRequired();
};

} // namespace audioapi
