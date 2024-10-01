#pragma once

#include "AudioNode.h"

namespace audioapi {

// using namespace facebook;
// using namespace facebook::jni;
//
// class AudioDestinationNode
//     : public jni::HybridClass<AudioDestinationNode, AudioNode> {
//  public:
//   static auto constexpr kJavaDescriptor =
//       "Lcom/swmansion/audioapi/nodes/AudioDestinationNode;";
// };

class AudioDestinationNode : public AudioNode {};

} // namespace audioapi
