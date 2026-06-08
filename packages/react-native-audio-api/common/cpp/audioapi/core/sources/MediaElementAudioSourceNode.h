#pragma once

#include <audioapi/core/AudioNode.h>
#include <cstdint>
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

  [[nodiscard]] uint64_t getBindingId() const {
    return bindingId_;
  }

  size_t getFileSourceNodeUseCount() const;
  bool fileSourceNodePaused() const;

  /// @note Audio Thread only — called after graph disconnects are applied.
  void onOutputsDisconnected();

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  static uint64_t generateBindingId();

  const uint64_t bindingId_;
  std::shared_ptr<AudioFileSourceNode> fileSource_;
};

} // namespace audioapi
