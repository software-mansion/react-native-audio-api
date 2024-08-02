#ifndef ANDROID
#include "AudioNodeWrapper.h"

namespace audiocontext
{
    void AudioNodeWrapper::connect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->connect(node->node_);
   }

    void AudioNodeWrapper::disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->disconnect(node->node_);
    }

    int AudioNodeWrapper::getNumberOfInputs() const {
        throw std::runtime_error("[AudioNodeHostObject] getNumberOfInputs not implemented yet!");
    }
    int AudioNodeWrapper::getNumberOfOutputs() const {
        throw std::runtime_error("[AudioNodeHostObject] getNumberOfOutputs not implemented yet!");
    }
} // namespace audiocontext
#endif
