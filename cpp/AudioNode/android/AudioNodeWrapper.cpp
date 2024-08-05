#ifdef ANDROID
#include "AudioNodeWrapper.h"

namespace audiocontext {

    AudioNodeWrapper::AudioNodeWrapper(const std::shared_ptr<AudioNode> &node) : node_(node) {
        numberOfInputs_ = node->getNumberOfInputs();
        numberOfOutputs_ = node->getNumberOfOutputs();
    }

    AudioNodeWrapper::~AudioNodeWrapper() {
        node_->prepareForDeconstruction();
    }

    int AudioNodeWrapper::getNumberOfInputs() const {
        return numberOfInputs_;
    }

    int AudioNodeWrapper::getNumberOfOutputs() const {
        return numberOfOutputs_;
    }

    void AudioNodeWrapper::connect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->connect(node->node_);
   }

    void AudioNodeWrapper::disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->disconnect(node->node_);
    }
}

#endif
