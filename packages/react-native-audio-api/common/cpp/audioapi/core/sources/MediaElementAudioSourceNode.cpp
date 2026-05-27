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
    const std::shared_ptr<AudioFileSourceNode> &fileSource,
    const MediaElementAudioSourceOptions &options)
    : AudioNode(context, options), bindingId_(generateBindingId()), fileSource_(fileSource) {
  isInitialized_.store(true, std::memory_order_release);
}

MediaElementAudioSourceNode::~MediaElementAudioSourceNode() {
  if (fileSource_ != nullptr) {
    fileSource_->releaseMediaElementSource(bindingId_);
  }
}

std::shared_ptr<DSPAudioBuffer> MediaElementAudioSourceNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (fileSource_ == nullptr) {
    processingBuffer->zero();
    return processingBuffer;
  }

  if (!fileSource_->isCurrentMediaElementSource(bindingId_)) {
    processingBuffer->zero();
    return processingBuffer;
  }

  return fileSource_->processDecodedOutput(processingBuffer, framesToProcess);
}

size_t MediaElementAudioSourceNode::getFileSourceNodeUseCount() const {
  if (fileSource_ == nullptr) {
    return 0;
  }
  return fileSource_.use_count();
}

bool MediaElementAudioSourceNode::fileSourceNodePaused() const {
  if (fileSource_ == nullptr) {
    return false;
  }
  return fileSource_->filePaused();
}

void MediaElementAudioSourceNode::onOutputsDisconnected() {
  if (fileSource_ == nullptr || !outputNodes_.empty()) {
    return;
  }

  if (fileSource_->isCurrentMediaElementSource(bindingId_)) {
    fileSource_->releaseMediaElementSource(bindingId_);
  }
}

} // namespace audioapi
