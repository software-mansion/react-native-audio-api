#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/events/AudioEventPayload.h>
#include <memory>
#include <string>

namespace audioapi {

AudioFileWriter::AudioFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : errorEvent_(audioEventHandlerRegistry), fileProperties_(fileProperties) {}

void AudioFileWriter::assignOnErrorCallbackId(uint64_t callbackId) {
  errorEvent_.assignCallbackId(callbackId);
}

void AudioFileWriter::invokeOnErrorCallback(const std::string &message) {
  errorEvent_.dispatch(StringPayload{.name = "message", .reason = message});
}

bool AudioFileWriter::isFileOpen() {
  return isFileOpen_.load(std::memory_order_acquire);
}

} // namespace audioapi
