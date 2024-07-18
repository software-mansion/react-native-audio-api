#include "AudioContext.h"

namespace audiocontext
{

  using namespace facebook::jni;

  AudioContext::AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis,
                             jlong jsContext) : javaObject_(make_global(jThis)), jsContext_(jsContext)
  {
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsContext);
    auto hostObject = std::make_shared<AudioContextHostObject>(this);
    auto object = jsi::Object::createFromHostObject(*runtime, hostObject);

    runtime->global().setProperty(*runtime, "__AudioContextProxy", object);
  }

  jsi::Object AudioContext::createOscillator()
  {
    static const auto method = javaClassLocal()->getMethod<OscillatorNode()>("createOscillator");
    auto oscillator = method(javaObject_.get());
    auto oscillatorCppInstance = oscillator->cthis();

    return oscillatorCppInstance->createOscillatorNodeHostObject();
  }

  jsi::Object AudioContext::getDestination()
  {
    static const auto method = javaClassLocal()->getMethod<AudioDestinationNode()>("getDestination");
    auto destination = method(javaObject_.get());
    auto destinationCppInstance = destination->cthis();

    return destinationCppInstance->createAudioDestinationHostObject();
  }

} // namespace audiocontext
