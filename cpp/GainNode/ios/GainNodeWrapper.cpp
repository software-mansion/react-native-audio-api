#ifndef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext {
    void GainNodeWrapper::setGain(double gain) {
        gain_->setGain(gain);
    }

    double GainNodeWrapper::getGain() {
        return gain_->getGain();
    }
}
#endif
