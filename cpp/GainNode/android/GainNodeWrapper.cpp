#ifdef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext {

    double GainNodeWrapper::getGain() {
        return gain_->getGain();
    }

    void GainNodeWrapper::setGain(double gain) {
        gain_->setGain(gain);
    }
}

#endif
