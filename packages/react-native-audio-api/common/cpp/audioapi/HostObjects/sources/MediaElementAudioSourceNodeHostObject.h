#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <memory>

namespace audioapi {

class MediaElementAudioSourceNode;

class MediaElementAudioSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit MediaElementAudioSourceNodeHostObject(
      const std::shared_ptr<MediaElementAudioSourceNode> &node);
  ~MediaElementAudioSourceNodeHostObject() override;
};

} // namespace audioapi
