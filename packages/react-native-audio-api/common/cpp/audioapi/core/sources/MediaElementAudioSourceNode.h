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
      AudioFileSourceNode *fileSource,
      const MediaElementAudioSourceOptions &options);

  ~MediaElementAudioSourceNode() override;

  [[nodiscard]] uint64_t getBindingId() const {
    return bindingId_;
  }

  size_t getFileSourceNodeUseCount() const;
  bool fileSourceNodePaused() const;
  bool canBeDestructed() const override;

  /// @note Audio Thread only — called after graph disconnects are applied.
  void onOutputsDisconnected();

 protected:
  void processNode(int framesToProcess) override;

 private:
  static uint64_t generateBindingId();

  const uint64_t bindingId_;
  AudioFileSourceNode *fileSource_;
};

} // namespace audioapi
