#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioScheduledSourceNode;

class AudioScheduledSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioScheduledSourceNodeHostObject(
      const std::shared_ptr<utils::graph::Graph> &graph,
      std::unique_ptr<AudioNode> node,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions());

  ~AudioScheduledSourceNodeHostObject() override;

  JSI_PROPERTY_SETTER_DECL(onEnded);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);

 protected:
  AudioScheduledSourceNode *scheduledSourceNode_ = nullptr;
};
} // namespace audioapi
