#include <audioapi/HostObjects/AudioConsumerHostObject.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioConsumerHostObject::AudioConsumerHostObject(std::shared_ptr<AudioConsumer> consumer)
    : consumer_(std::move(consumer)) {}

// Key function: this out-of-line definition pins the vtable and typeinfo to this library, so an
// extension's getHostObject<AudioConsumerHostObject>() finds the same type. Keep it out-of-line.
AudioConsumerHostObject::~AudioConsumerHostObject() {
  if (consumer_ != nullptr) {
    consumer_->invalidate();
  }
}

} // namespace audioapi
