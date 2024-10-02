#pragma once

#include <cmath>
#include <string>
#include <memory>

#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

using namespace oboe;

class OscillatorNode : public AudioStreamDataCallback,
                       public AudioScheduledSourceNode {
 public:
  explicit OscillatorNode();

  std::shared_ptr<AudioParam> getFrequencyParam() const;
  std::shared_ptr<AudioParam> getDetuneParam() const;
  std::string getType();
  void setType(const std::string &type);

  DataCallbackResult onAudioReady(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames) override;

 private:
  std::shared_ptr<AudioParam> frequencyParam_;
  std::shared_ptr<AudioParam> detuneParam_;
  std::string type_ = "sine";
  float phase_ = 0.0;
};
} // namespace audioapi
