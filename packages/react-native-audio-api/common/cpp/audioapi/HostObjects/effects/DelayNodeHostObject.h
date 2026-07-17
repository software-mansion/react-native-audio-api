#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct DelayOptions;
class BaseAudioContext;
class AudioParamHostObject;
class DelayNode;
class DelayLine;
class DelayWriterHostNode;
class DelayReaderHostNode;

class DelayNodeHostObject : public AudioNodeHostObject {
 public:
  explicit DelayNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const DelayOptions &options);

  [[nodiscard]] size_t getMemoryPressure() const override;

  JSI_PROPERTY_GETTER_DECL(delayTime);
  std::shared_ptr<DelayWriterHostNode> delayWriterHostNode_;
  std::shared_ptr<DelayReaderHostNode> delayReaderHostNode_;

 protected:
  // A delay is a composite node: audio written through the writer sub-node is
  // read back (delayed) through the reader sub-node. Outgoing edges therefore
  // originate at the reader, incoming edges terminate at the writer.
  std::shared_ptr<utils::graph::HostNode> getConnectSource(int outputIndex) override;
  std::shared_ptr<utils::graph::HostNode> getConnectDestination(int inputIndex) override;

 private:
  DelayNode *delayNode_ = nullptr;

  std::shared_ptr<AudioParamHostObject> delayTimeParam_;
  std::shared_ptr<DelayLine> delayLine_;
};
} // namespace audioapi
