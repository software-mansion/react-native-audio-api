#ifdef ANDROID
#include "AudioNodeWrapper.h"

namespace audiocontext {

    AudioNodeWrapper::AudioNodeWrapper(AudioNode *node) : node_(node) {
        numberOfInputs_ = node->getNumberOfInputs();
        numberOfOutputs_ = node->getNumberOfOutputs();
    }

    AudioNodeWrapper::~AudioNodeWrapper() {
        node_->resetJavaPart();
    }

    int AudioNodeWrapper::getNumberOfInputs() const {
        return numberOfInputs_;
    }

    int AudioNodeWrapper::getNumberOfOutputs() const {
        return numberOfOutputs_;
    }

    int AudioNodeWrapper::getChannelCount() const {
        return node_->getChannelCount();
    }

    std::string AudioNodeWrapper::getChannelCountMode() const {
        return node_->getChannelCountMode();
    }

    std::string AudioNodeWrapper::getChannelInterpretation() const {
        return node_->getChannelInterpretation();
    }

    void AudioNodeWrapper::connect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->connect(node->node_);
   }

    void AudioNodeWrapper::disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->disconnect(node->node_);
    }
}

#endif
