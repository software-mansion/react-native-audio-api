#ifndef ANDROID
#include "BiquadFilterWrapper"
namespace audiocontext
{
    // TODO: Add BiquadFilterNode implementation

    BiquadFilterNodeWrapper::BiquadFilterNodeWrapper() {}

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getFrequencyParam() {
        return null;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getDetuneParam() {
        return null;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getQParam() {
        return null;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getGainParam() {
        return null;
    }

    std::string BiquadFilterNodeWrapper::getType() {
        return null;
    }

    void BiquadFilterNodeWrapper::setType(const std::string& filterType) {
    }
} // namespace audiocontext
#endif
