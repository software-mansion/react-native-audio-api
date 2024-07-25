#ifndef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext {
    void GainNodeWrapper::setGain(double gain) {
        gainNode_->changeGain(gain);
    }

    double GainNodeWrapper::getGain() {
        return gainNode_->getGain();
    }
}
#endif
