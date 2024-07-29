#ifndef ANDROID
#include "AudioNodeWrapper.h"

namespace audiocontext {

    void AudioNodeWrapper::connect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->connect(node->node_);
   }

    void AudioNodeWrapper::disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const {
        node_->disconnect(node->node_);
    }
}
#endif
