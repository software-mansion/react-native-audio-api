#pragma once

#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <audioapi/encoding/AudioEncoder.h>

#include <cstdint>
#include <memory>
#include <string>

namespace audioapi {

/// Android recorder file writer backed by the system AudioEncoder.
class OsAudioFileWriter : public AndroidFileWriterBackend {
 public:
  OsAudioFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t streamMaxBufferSize);
  ~OsAudioFileWriter() override;

  size_t getFileSizeBytes() const override;

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t streamMaxBufferSize,
      const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

 private:
  void processWriterData(void *data, int numFrames) override;
  void rollbackFailedOpen();

  std::unique_ptr<AudioEncoder> encoder_;
};

} // namespace audioapi
