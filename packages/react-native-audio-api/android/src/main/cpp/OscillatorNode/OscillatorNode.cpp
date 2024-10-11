#include "OscillatorNode.h"
#include "AudioContext.h"

namespace audioapi {

OscillatorNode::OscillatorNode(AudioContext *context)
    : AudioScheduledSourceNode(context) {
  frequencyParam_ = std::make_shared<AudioParam>(
      context, 444.0, -NYQUIST_FREQUENCY, NYQUIST_FREQUENCY);
  detuneParam_ =
      std::make_shared<AudioParam>(context, 0.0, -MAX_DETUNE, MAX_DETUNE);
}

std::shared_ptr<AudioParam> OscillatorNode::getFrequencyParam() const {
  return frequencyParam_;
}

std::shared_ptr<AudioParam> OscillatorNode::getDetuneParam() const {
  return detuneParam_;
}

std::string OscillatorNode::getType() {
  return OscillatorNode::toString(type_);
}

void OscillatorNode::setType(const std::string &type) {
  type_ = OscillatorNode::fromString(type);
}

bool OscillatorNode::processAudio(float *audioData, int32_t numFrames) {
    if (!isPlaying_) {
        return false;
    } else {
        auto time = context_->getCurrentTime();
        auto deltaTime = 1 / context_->getSampleRate();


        for (int i = 0; i < numFrames; ++i) {
            auto detuneRatio = std::pow(
                    2.0f,
                    detuneParam_->getValueAtTime(time) / 1200.0f);
            auto detunedFrequency =
                    frequencyParam_->getValueAtTime(time) *
                    detuneRatio;
            auto phaseIncrement =
                    static_cast<float>(2 * M_PI * detunedFrequency / context_->getSampleRate());
            float value = OscillatorNode::getWaveBufferElement(phase_, type_);

            for (int j = 0; j < channelCount_; j++) {
                audioData[i * channelCount_ + j] = value;
            }

            phase_ += phaseIncrement;
            time += deltaTime;

            if (phase_ >= 2 * M_PI) {
                phase_ -= 2 * M_PI;
            }
        }

        return true;
    }
}
} // namespace audioapi
