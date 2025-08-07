#include <audioapi/HostObjects/BaseAudioContextHostObject.h>

// Explicitly define destructors here, as they to exist in order to act as a
// "key function" for the audio classes - this allow for RTTI to work
// properly across dynamic library boundaries (i.e. dynamic_cast that is used by
// isHostObject method), android specific issue
namespace audioapi {
BaseAudioContextHostObject::~BaseAudioContextHostObject() = default;
}