#pragma once

#include <audioapi/core/AudioNode.h>
#include <memory>

namespace audioapi {

class AudioFileSourceNode;
struct MediaElementAudioSourceOptions;

class MediaElementAudioSourceNode : public AudioNode {
 public:
  explicit MediaElementAudioSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const std::shared_ptr<AudioFileSourceNode> &fileSource,
      const MediaElementAudioSourceOptions &options);

  ~MediaElementAudioSourceNode() override;

  size_t getFileSourceNodeUseCount() const;
  bool fileSourceNodePaused() const;
  void disconnect(const std::shared_ptr<AudioNode> &node) override;
  void disconnect() override;

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioFileSourceNode> fileSource_;
};

} // namespace audioapi
