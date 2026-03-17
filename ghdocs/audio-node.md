# How to create new AudioNode

In this docs we present recommended patterns for creating new AudioNodes.

## Layers

Ususally, each AudioNode has three layers:

- Core (C++)

  Class implementing core audio processing logic. Should be implemented in highly performant manner using internal data structures (if possible).

```cpp
class GainNode : public AudioNode {
 public:
  explicit GainNode(const std::shared_ptr<BaseAudioContext> &context, const GainOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};
```

- Host Object (HO)

  Interop class between C++ and JS, implemented on C++ side. HO is returned from C++ to JS from BaseAudioContext factory methods. JS has its own interfaces that works as a counterpart of C++ HO. There is no strong typing mechanism between C++ and JS. Implementation is based on the alignment between C++ HO and JS interface.

```cpp
class GainNodeHostObject : public AudioNodeHostObject {
 public:
  explicit GainNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const GainOptions &options);

  JSI_PROPERTY_GETTER_DECL(gain);

 private:
  std::shared_ptr<AudioParamHostObject> gainParam_;
};
```
```ts
export interface IGainNode extends IAudioNode {
  readonly gain: IAudioParam;
}
```

- Typescript (JS)

  Elegant typescript wrapper around JS HO interface.

```ts
class GainNode extends AudioNode {
  readonly gain: AudioParam;

  constructor(context: BaseAudioContext, options?: TGainOptions) {
    const gainNode: IGainNode = context.context.createGain(options || {}); // context.context is C++ HO
    super(context, gainNode);
    this.gain = new AudioParam(gainNode.gain, context);
  }
}
```

## Core (C++) implementation

Each AudioNode should implement one virtual method:
```cpp
std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess)
```

It is responsible for AudioNode's processing logic. It gets input buffer as argument - `processingBus` and should return processed buffer.

```cpp
std::shared_ptr<AudioBuffer> GainNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBuffer;
  double time = context->getCurrentTime();
  auto gainParamValues = gainParam_->processARateParam(framesToProcess, time);
  auto gainValues = gainParamValues->getChannel(0);

  for (size_t i = 0; i < processingBuffer->getNumberOfChannels(); i++) {
    auto channel = processingBuffer->getChannel(i);
    channel->multiply(*gainValues, framesToProcess);
  }

  return processingBuffer;
}
```

There are a few rules that should be followed when implementing C++ AudioNode core.

- **Thread safety**: Each AudioNode should be created in thread safe manner.

- **Heap allocations**: Heap allocations are not allowed on the Audio Thread, so all necessary data should be allocated in constructor, or pre-allocated on other thread and passed to AudioNode.

- **Destructions** No destructions are allowed to happen on the Audio Thread. AudioNode destruction are handled by already active AudioDestructor. If you need to perform some cleanup, you have to delegate it to AudioDestructor.

- **No locks on Audio Thread**: Locks are not allowed on the Audio Thread. Audio procssing have to be highly performant and efficient.

- **No syscalls** Syscalls are not allowed on the Audio Thread, so if you need to perform some work that requires syscalls, you have to delegate it to other thread.

## HostObject implementation

We can distinguish three types of AudioNode's JS methods:

1. **getter**

```ts
const fftSize = analyserNode.fftSize;
```

C++ counter part is `JSI_PROPERTY_GETTER`. It just returns some value.

```cpp
JSI_PROPERTY_GETTER_IMPL(AnalyserNodeHostObject, fftSize) {
  return {fftSize_};
}
```

2. **setter**

C++ counterpart is `JSI_PROPERTY_SETTER`. It just receives some value.

```ts
analyserNode.fftSize = 2048;
```

```cpp
JSI_PROPERTY_SETTER_IMPL(AnalyserNodeHostObject, minDecibels) {
  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  auto minDecibels = static_cast<float>(value.getNumber());
  auto event = [analyserNode, minDecibels](BaseAudioContext&) {
    analyserNode->setMinDecibels(minDecibels);
  };
    analyserNode->scheduleAudioEvent(std::move(event));
    minDecibels_ = minDecibels;
}
```


3. **function**

C++ counterpart is `JSI_HOST_FUNCTION`. It is a common function that can receive arguments and return some value.

```ts
const fftOutput = new Uint8Array(analyser.frequencyBinCount);
analyserNode.getByteFrequencyData(fftOutput);
```

```cpp
JSI_HOST_FUNCTION_IMPL(AnalyserNodeHostObject, getByteFrequencyData) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto data = arrayBuffer.data(runtime);
  auto length = static_cast<int>(arrayBuffer.size(runtime));

  auto analyserNode = std::static_pointer_cast<AnalyserNode>(node_);
  analyserNode->getByteFrequencyData(data, length);

  return jsi::Value::undefined();
}
```

All methods should be registerd in C++ HO constructor:

```cpp
AnalyserNodeHostObject::AnalyserNodeHostObject(const std::shared_ptr<BaseAudioContext>& context, const AnalyserOptions &options)
    : /* ... */ {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(AnalyserNodeHostObject, fftSize));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(AnalyserNodeHostObject, fftSize));
  addFunctions(JSI_EXPORT_FUNCTION(AnalyserNodeHostObject, getByteFrequencyData),);
}
```

#### Shadow state (C++)

Shadow state is a mechanism introduced in order to make communication between JS and the Audio Thread lock-free. AudioNodeHostObject stores the set of properties, which are  modified only by JS thread (the same set C++ AudioNode has). Everytime we want to access some property from JS, we can just return property from shadow state, when we modify some property we have to update shadow state and schedule update event on Audio Event Loop (SPSC). By following that manner we can skip accessing AudioNode state, that is also accessed by the Audio Thread - no need to lock or use atomic variables.

```cpp
class OscillatorNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  /* ... */

  JSI_PROPERTY_GETTER_DECL(type);
  JSI_PROPERTY_SETTER_DECL(type);

 private:
  /* ... */
  OscillatorType type_;
};
```

```cpp
JSI_PROPERTY_GETTER_IMPL(OscillatorNodeHostObject, type) {
  return jsi::String::createFromUtf8(runtime, js_enum_parser::oscillatorTypeToString(type_));
}

JSI_PROPERTY_SETTER_IMPL(OscillatorNodeHostObject, type) {
  auto oscillatorNode = std::static_pointer_cast<OscillatorNode>(node_);
  auto type = js_enum_parser::oscillatorTypeFromString(value.asString(runtime).utf8(runtime));

  auto event = [oscillatorNode, type](BaseAudioContext &) {
    oscillatorNode->setType(type);
  };
  type_ = type;

  oscillatorNode->scheduleAudioEvent(std::move(event));
}
```

#### Communication between JS Thread and Audio Thread

**getters** and **setters**

1. Property is primitive and is not modified by the Audio Thread.

   Shadow state design pattern should be followed.

2. Property is not primitive and is not modified by the Audio Thread.

  It should be stored in TS layer and copied to AudioNode

3. Property is primitive and can be modified by the Audio Thread.

  In C++ core it should be an atomic variable that allows to access it in thread-safe manner from both threads.

```cpp
class AudioParam {
public:
  /* ... */

  [[nodiscard]] inline float getValue() const noexcept {
    return value_.load(std::memory_order_relaxed);
  }

  inline void setValue(float value) {
    value_.store(std::clamp(value, minValue_, maxValue_), std::memory_order_release);
  }

  /* ... */

private:
  std::atomic<float> value_;

  /* ... */
};
```

```cpp
JSI_PROPERTY_GETTER_IMPL(AudioParamHostObject, value) {
  return {param_->getValue()};
}

JSI_PROPERTY_SETTER_IMPL(AudioParamHostObject, value) {
    auto event = [param = param_, value = static_cast<float>(value.getNumber())](BaseAudioContext &) {
        param->setValue(value);
    };

    param_->scheduleAudioEvent(std::move(event));
}
```

4. Property is not primitive and can be modified by the Audio Thread.
  In C++ core triple buffer pattern should be followed. It allows to have one copy of property for reader, one for writer and one for pending update. On each update we just swap pending update with writer, and on each read we just read from reader. In that manner we can skip locks and just operate on atomic indices.

  Check AnalyserNode implementation for example or this [article](https://medium.com/@sgn00/triple-buffer-lock-free-concurrency-primitive-611848627a1e) for more details.

**functions**

Function's should follow the same thread-safe lock-free patterns as getters/setters. Set of properties read/write by the function determines mechanisms that should be used in implementation.
