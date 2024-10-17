#pragma once

#include <memory>
#include <string>

#include "AudioNode.h"

namespace audioapi {

class AudioNodeWrapper {

 public:
  explicit AudioNodeWrapper(const std::shared_ptr<AudioNode> &node);

    int getNumberOfInputs() const;
    int getNumberOfOutputs() const;
    int getChannelCount() const;
    std::string getChannelCountMode() const;
    std::string getChannelInterpretation() const;
    void connect(const std::shared_ptr<AudioNodeWrapper> &node) const;
    void disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const;

 protected:
  std::shared_ptr<AudioNode> node_;
};
} // namespace audioapi
