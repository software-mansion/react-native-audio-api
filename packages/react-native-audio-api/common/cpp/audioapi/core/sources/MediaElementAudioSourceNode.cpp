#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

MediaElementAudioSourceNode::MediaElementAudioSourceNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const std::shared_ptr<AudioFileSourceNode> &fileSource,
    const MediaElementAudioSourceOptions &options)
    : AudioNode(context, options), fileSource_(fileSource) {
  isInitialized_.store(true, std::memory_order_release);
}

MediaElementAudioSourceNode::~MediaElementAudioSourceNode() {
  if (fileSource_ != nullptr) {
    fileSource_->onMediaElementSourceReleased();
  }
}

std::shared_ptr<DSPAudioBuffer> MediaElementAudioSourceNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (fileSource_ == nullptr) {
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

void MediaElementAudioSourceNode::disconnect() {
  fileSource_->onMediaElementSourceReleased();
  AudioNode::disconnect();
}

void MediaElementAudioSourceNode::disconnect(const std::shared_ptr<AudioNode> &node) {
  if (outputNodes_.empty()) {
    fileSource_->onMediaElementSourceReleased();
  }
  AudioNode::disconnect(node);
}

} // namespace audioapi
