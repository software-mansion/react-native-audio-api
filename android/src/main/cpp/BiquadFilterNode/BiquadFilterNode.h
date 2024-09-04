#pragma once

#include <string>
#include "AudioNode.h"
#include "AudioParam.h"

namespace audiocontext {

using namespace facebook;
using namespace facebook::jni;

class BiquadFilterNode : public jni::HybridClass<BiquadFilterNode, AudioNode> {
 public:
  static auto constexpr kJavaDescriptor =
      "Lcom/audiocontext/nodes/filter/BiquadFilterNode;";

  AudioParam *getFrequencyParam();
  AudioParam *getDetuneParam();
  AudioParam *getQParam();
  AudioParam *getGainParam();
  std::string getFilterType();
  void setFilterType(const std::string &filterType);
};

} // namespace audiocontext
