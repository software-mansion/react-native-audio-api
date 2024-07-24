#ifndef ANDROID
#include "GainNodeWrapper.h"

namespace audiocontext {
    void GainNodeWrapper::setGain(double gain) {
        gainNode_->changeGain(gain);
    }
}
#endif
