#include "AudioContext.h"
#include "android/log.h"

namespace audiocontext
{

  using namespace facebook::jni;

  AudioContext::AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis,
                             jlong jsContext) : javaObject_(make_global(jThis)), jsContext_(jsContext)
  {
    auto audioContextWrapper = std::make_shared<AudioContextWrapper>(std::shared_ptr<AudioContext>(this));
    auto runtime = reinterpret_cast<jsi::Runtime *>(jsContext);
    auto hostObject = std::make_shared<AudioContextHostObject>(audioContextWrapper);

    auto object = jsi::Object::createFromHostObject(*runtime, hostObject);
    runtime->global().setProperty(*runtime, "__AudioContextProxy", std::move(object));
  }

  std::shared_ptr<OscillatorNodeHostObject> AudioContext::createOscillator()
  {
    static const auto method = javaClassLocal()->getMethod<OscillatorNode()>("createOscillator");
    auto oscillator = method(javaObject_.get());
    auto oscillatorCppInstance = oscillator->cthis();

    return oscillatorCppInstance->createOscillatorNodeHostObject();
  }

    std::shared_ptr<AudioDestinationNodeHostObject> AudioContext::getDestination()
  {
    static const auto method = javaClassLocal()->getMethod<AudioDestinationNode()>("getDestination");
    auto destination = method(javaObject_.get());
    auto destinationCppInstance = destination->cthis();

    return destinationCppInstance->createAudioDestinationHostObject();
  }

} // namespace audiocontext
