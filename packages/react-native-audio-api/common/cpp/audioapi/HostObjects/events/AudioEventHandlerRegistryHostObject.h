#pragma once

#include <audioapi/jsi/HostObject.h>

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class IAudioEventHandlerRegistry;

class AudioEventHandlerRegistryHostObject : public HostObject {
 public:
  explicit AudioEventHandlerRegistryHostObject(
      const std::shared_ptr<IAudioEventHandlerRegistry> &eventHandlerRegistry);

  /// Vtable anchor — deliberately declared here and defined out-of-line in the .cpp.
  /// Without a key function the vtable and typeinfo are emitted as weak symbols in every
  /// translation unit that touches this class, so a consumer linked into a different .so
  /// (react-native-audio-worklets, built into libappmodules.so) ends up with its own type
  /// identity and jsi's dynamic_cast-based isHostObject<T>() fails to recognise instances
  /// created here. Pinning them to this TU gives every consumer one shared identity.
  ~AudioEventHandlerRegistryHostObject() override;

  JSI_HOST_FUNCTION_DECL(addAudioEventListener);
  JSI_HOST_FUNCTION_DECL(removeAudioEventListener);

  [[nodiscard]] const std::shared_ptr<IAudioEventHandlerRegistry> &getEventHandlerRegistry() const {
    return eventHandlerRegistry_;
  }

 private:
  std::shared_ptr<IAudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi
