#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/types/NodeOptions.h>

#include <audioapi/utils/Macros.h>
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
  DELETE_COPY_AND_MOVE(AudioScheduledSourceNodeHostObject);

  JSI_PROPERTY_SETTER_DECL(onEnded);

  virtual JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);

 protected:
  AudioScheduledSourceNode *const scheduledSourceNode_;
};
} // namespace audioapi
