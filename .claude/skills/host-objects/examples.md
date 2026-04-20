# HostObject Examples

## GainNodeHostObject (no shadow state)

`gain` is an `AudioParam` — no shadow state needed because its `value_` is `std::atomic<float>`.

### `effects/GainNodeHostObject.h`

```cpp
#pragma once
#include "audioapi/HostObjects/AudioNodeHostObject.h"
#include "audioapi/HostObjects/AudioParamHostObject.h"
#include "audioapi/core/effects/GainNode.h"

namespace audioapi {

class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const GainOptions &options);

  // JS-thread only
  JSI_PROPERTY_GETTER_DECL(gain);
};

} // namespace audioapi
```

### `effects/GainNodeHostObject.cpp`

```cpp
GainNodeHostObject::GainNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const GainOptions &options)
    : AudioNodeHostObject(context->createGain(options)) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(GainNodeHostObject, gain));
}

JSI_PROPERTY_GETTER_IMPL(GainNodeHostObject, gain) {
  auto gainNode  = std::static_pointer_cast<GainNode>(node_);
  auto gainParam = std::make_shared<AudioParamHostObject>(gainNode->getGainParam());
  return jsi::Object::createFromHostObject(runtime, gainParam);
}
```

---

## OscillatorNodeHostObject (with shadow state)

`type` is a plain enum — not atomic — so it uses shadow state. `AudioParam` wrappers for `frequency` and `detune` are preallocated in the constructor and returned directly (no shadow needed since the underlying `value_` is atomic on the node).

### `sources/OscillatorNodeHostObject.h`

```cpp
#pragma once
#include "audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h"
#include "audioapi/core/sources/OscillatorNode.h"

namespace audioapi {

class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit OscillatorNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const OscillatorOptions &options);

  // JS-thread only
  JSI_PROPERTY_GETTER_DECL(frequency);
  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(type);
  JSI_PROPERTY_SETTER_DECL(type);
  JSI_HOST_FUNCTION_DECL(setPeriodicWave);

 private:
  // Shadow state — JS thread only
  OscillatorType type_;
  // AudioParam wrappers — not shadow state, wrap atomic state on the node
  std::shared_ptr<AudioParamHostObject> frequencyParam_;
  std::shared_ptr<AudioParamHostObject> detuneParam_;
};

} // namespace audioapi
```

### `sources/OscillatorNodeHostObject.cpp`

```cpp
OscillatorNodeHostObject::OscillatorNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const OscillatorOptions &options)
    : AudioScheduledSourceNodeHostObject(context->createOscillator(options)) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);

  // Initialize shadow from options
  type_ = options.type;

  // Preallocate AudioParam wrappers
  frequencyParam_ = std::make_shared<AudioParamHostObject>(oscillatorNode->getFrequencyParam());
  detuneParam_    = std::make_shared<AudioParamHostObject>(oscillatorNode->getDetuneParam());

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, frequency),
      JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, detune),
      JSI_EXPORT_PROPERTY_GETTER(OscillatorNodeHostObject, type));
  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(OscillatorNodeHostObject, type));
  addFunctions(
      JSI_EXPORT_FUNCTION(OscillatorNodeHostObject, setPeriodicWave));
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, frequency) {
  return jsi::Object::createFromHostObject(runtime, frequencyParam_);
}

JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, type) {
  // Return shadow — never read from node_
  return jsi::String::createFromUtf8(
      runtime, js_enum_parser::oscillatorTypeToString(type_));
}

JSI_PROPERTY_SETTER_IMPL(OscillatorNodeHostObject, type) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto type = js_enum_parser::oscillatorTypeFromString(
      value.asString(runtime).utf8(runtime));

  // 1. Shadow update — immediate, JS thread
  type_ = type;

  // 2. Audio thread update — async, lock-free
  auto event = [oscillatorNode, type](BaseAudioContext &) {
    oscillatorNode->setType(type);
  };
  oscillatorNode->scheduleAudioEvent(std::move(event));
}

JSI_HOST_FUNCTION_IMPL(OscillatorNodeHostObject, setPeriodicWave) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto periodicWave   = args[0].getObject(runtime)
      .getHostObject<PeriodicWaveHostObject>(runtime);
  oscillatorNode->setPeriodicWave(periodicWave->periodicWave_);
  return jsi::Value::undefined();
}
```

