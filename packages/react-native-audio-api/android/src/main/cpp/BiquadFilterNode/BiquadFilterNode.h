#pragma once

#include <string>
#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

    //TODO: Implement this class

// using namespace facebook;
// using namespace facebook::jni;
//
// class BiquadFilterNode : public jni::HybridClass<BiquadFilterNode, AudioNode>
// {
//  public:
//   static auto constexpr kJavaDescriptor =
//       "Lcom/swmansion/audioapi/nodes/filter/BiquadFilterNode;";
//
//   AudioParam *getFrequencyParam();
//   AudioParam *getDetuneParam();
//   AudioParam *getQParam();
//   AudioParam *getGainParam();
//   std::string getFilterType();
//   void setFilterType(const std::string &filterType);
// };

class BiquadFilterNode : public AudioNode {};

} // namespace audioapi
