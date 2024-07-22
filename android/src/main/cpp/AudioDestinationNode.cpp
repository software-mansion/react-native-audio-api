#include "AudioDestinationNode.h"

namespace audiocontext {

    using namespace facebook::jni;

    AudioDestinationNode::AudioDestinationNode(jni::alias_ref<AudioDestinationNode::jhybridobject> &jThis): javaObject_(make_global(jThis)) {}

} // namespace audiocontext
