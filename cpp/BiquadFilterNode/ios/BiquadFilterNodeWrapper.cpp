#ifndef ANDROID
#include "BiquadFilterWrapper"
namespace audiocontext
{
    // TODO: Add BiquadFilterNode implementation

    BiquadFilterNodeWrapper::BiquadFilterNodeWrapper() {}

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getFrequencyParam() const {
        return null;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getDetuneParam() const {
        return null;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getQParam() const {
        return null;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getGainParam() const {
        return null;
    }

    std::string BiquadFilterNodeWrapper::getType() {
        return null;
    }

    void BiquadFilterNodeWrapper::setType(const std::string& filterType) {
    }
} // namespace audiocontext
#endif
