#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <atomic>
#include <memory>

namespace audioapi {

uint64_t MediaElementAudioSourceNode::generateBindingId() {
  static std::atomic<uint64_t> nextMediaElementBindingId{1};
  return nextMediaElementBindingId.fetch_add(1, std::memory_order_relaxed);
}

MediaElementAudioSourceNode::MediaElementAudioSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    AudioFileSourceNode *fileSource,
    const MediaElementAudioSourceOptions &options)
    : AudioNode(context, options), bindingId_(generateBindingId()), fileSource_(fileSource) {
  if (fileSource_ != nullptr) {
    fileSource_->bindMediaElementSource(bindingId_);
  }
}

MediaElementAudioSourceNode::~MediaElementAudioSourceNode() {
  if (fileSource_ != nullptr) {
    fileSource_->releaseMediaElementSource(bindingId_);
  }
}

bool MediaElementAudioSourceNode::canBeDestructed() const {
  return fileSourceNodePaused();
}

void MediaElementAudioSourceNode::onOutputsDisconnected() {
  if (fileSource_ != nullptr) {
    fileSource_->releaseMediaElementSource(bindingId_);
  }
}

void MediaElementAudioSourceNode::processNode(int framesToProcess) {
  if (fileSource_ == nullptr) {
    audioBuffer_->zero();
    return;
  }

  if (!fileSource_->isCurrentMediaElementSource(bindingId_)) {
    audioBuffer_->zero();
    return;
  }

  fileSource_->processDecodedOutput(framesToProcess, audioBuffer_);
}

size_t MediaElementAudioSourceNode::getFileSourceNodeUseCount() const {
  return fileSource_ != nullptr ? 1 : 0;
}

bool MediaElementAudioSourceNode::fileSourceNodePaused() const {
  if (fileSource_ == nullptr) {
    return true;
  }
  return fileSource_->filePaused();
}

} // namespace audioapi
