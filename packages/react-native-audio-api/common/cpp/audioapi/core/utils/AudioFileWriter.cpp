#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace audioapi {

AudioFileWriter::AudioFileWriter(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    const std::shared_ptr<AudioFileProperties> &fileProperties)
    : fileProperties_(fileProperties), audioEventHandlerRegistry_(audioEventHandlerRegistry) {}

void AudioFileWriter::setOnErrorCallback(uint64_t callbackId) {
  errorCallbackId_.store(callbackId, std::memory_order_release);
}

void AudioFileWriter::clearOnErrorCallback() {
  errorCallbackId_.store(0, std::memory_order_release);
}

void AudioFileWriter::invokeOnErrorCallback(const std::string &message) {
  uint64_t callbackId = errorCallbackId_.load(std::memory_order_acquire);

  // TODO: only the line above is atomic, which means that between reading the callbackId and invoking the callback,
  // the callback could be cleared. We need to ensure that the callback is still valid when invoking it.
  // TL;DR: atomic szpont
  if (audioEventHandlerRegistry_ == nullptr || callbackId == 0) {
    return;
  }

  std::unordered_map<std::string, EventValue> eventPayload = {};
  eventPayload.insert({"message", message});
  audioEventHandlerRegistry_->invokeHandlerWithEventBody("error", callbackId, eventPayload);
}

bool AudioFileWriter::isFileOpen() {
  return isFileOpen_.load(std::memory_order_acquire);
}

} // namespace audioapi
