#include <audioapi/android/core/utils/AndroidFileWriterBackend.h>
#include <memory>
#include <utility>

namespace audioapi {
AndroidFileWriterBackend::AndroidFileWriterBackend(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioFileWriter(audioEventHandlerRegistry, fileProperties) {
  auto [sender, receiver] = channels::spsc::channel<
      WriterData,
      AudioFileWriter::FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      AudioFileWriter::FILE_WRITER_SPSC_WAIT_STRATEGY>(
      AudioFileWriter::FILE_WRITER_CHANNEL_CAPACITY);
  sender_ = std::move(sender);
  receiver_ = std::move(receiver);
  stopFileWriterThread_.store(false, std::memory_order_release);
}

void AndroidFileWriterBackend::writeAudioData(void *data, int numFrames) {
  sender_.send({data, numFrames});
}

void AndroidFileWriterBackend::stopFileWriterThread() {
  stopFileWriterThread_.store(true, std::memory_order_release);
  sender_.send({nullptr, 0});
  if (fileWriterThread_.joinable()) {
    fileWriterThread_.join();
  }
}
} // namespace audioapi
