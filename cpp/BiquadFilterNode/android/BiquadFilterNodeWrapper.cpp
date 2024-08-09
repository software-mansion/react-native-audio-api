#ifdef ANDROID
#include "BiquadFilterNodeWrapper.h"

namespace audiocontext {

    std::shared_ptr<BiquadFilterNode> BiquadFilterNodeWrapper::getBiquadFilterNodeFromAudioNode() {
        return std::static_pointer_cast<BiquadFilterNode>(node_);
    }

    BiquadFilterNodeWrapper::BiquadFilterNodeWrapper(const std::shared_ptr<BiquadFilterNode> &biquadFilterNode)
            : AudioNodeWrapper(biquadFilterNode) {
        auto frequencyParam = biquadFilterNode->getFrequencyParam();
        frequencyParam_ = std::make_shared<AudioParamWrapper>(frequencyParam);
        auto detuneParam = biquadFilterNode->getDetuneParam();
        detuneParam_ = std::make_shared<AudioParamWrapper>(detuneParam);
        auto QParam = biquadFilterNode->getQParam();
        QParam_ = std::make_shared<AudioParamWrapper>(QParam);
        auto gainParam = biquadFilterNode->getGainParam();
        gainParam_ = std::make_shared<AudioParamWrapper>(gainParam);
    }


    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getFrequencyParam() {
        return frequencyParam_;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getDetuneParam() {
        return detuneParam_;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getQParam() {
        return QParam_;
    }

    std::shared_ptr<AudioParamWrapper> BiquadFilterNodeWrapper::getGainParam() {
        return gainParam_;
    }

    std::string BiquadFilterNodeWrapper::getType() {
        auto biquadFilterNode_ = getBiquadFilterNodeFromAudioNode();
        return biquadFilterNode_->getFilterType();
    }

    void BiquadFilterNodeWrapper::setType(const std::string& filterType) {
        auto biquadFilterNode_ = getBiquadFilterNodeFromAudioNode();
        biquadFilterNode_->setFilterType(filterType);
    }
} // namespace audiocontext
#endif
