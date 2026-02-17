#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <memory>
#include <utility>

namespace audioapi {
AndroidFileWriterBackend::AndroidFileWriterBackend(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties) {

  auto offloaderLambda = [this](WriterData data) {
    taskOffloaderFunction(data);
  };
  offloader_ = std::make_unique<task_offloader::TaskOffloader<
      WriterData,
      FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      FILE_WRITER_SPSC_WAIT_STRATEGY>>(FILE_WRITER_CHANNEL_CAPACITY, offloaderLambda);
}

void AndroidFileWriterBackend::writeAudioData(void *data, int numFrames) {
  offloader_->getSender()->send({data, numFrames});
}
} // namespace audioapi
