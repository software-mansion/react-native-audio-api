#ifndef ANDROID
#include "AudioNodeWrapper.h"
#include "iostream"

namespace audiocontext {

    void AudioNodeWrapper::connect(const std::shared_ptr<AudioNodeWrapper> node) const {
      throw std::runtime_error("[AudioNodeWrapper::connect] Not yet implemented!");
   }

    void AudioNodeWrapper::disconnect(const std::shared_ptr<AudioNodeWrapper> node) const {
			throw std::runtime_error("[AudioNodeWrapper::disconnect] Not yet implemented!");
    }
}
#endif
