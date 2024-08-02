#include "AudioContext.h"

namespace audiocontext
{

  using namespace facebook::jni;

    AudioContext::AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis) : javaObject_(make_global(jThis)){}

    void AudioContext::install(jlong jsContext)
    {
        auto audioContextWrapper = std::make_shared<AudioContextWrapper>(std::shared_ptr<AudioContext>(this));
        AudioContextHostObject::createAndInstallFromWrapper(audioContextWrapper, jsContext);
    }

    std::shared_ptr<OscillatorNode> AudioContext::createOscillator()
    {
        static const auto method = javaClassLocal()->getMethod<OscillatorNode()>("createOscillator");
        auto oscillator = method(javaObject_.get());
        auto oscillatorCppInstance = oscillator->cthis();

        return std::shared_ptr<OscillatorNode>(oscillatorCppInstance);
    }

    std::shared_ptr<AudioDestinationNode> AudioContext::getDestination()
    {
        static const auto method = javaClassLocal()->getMethod<AudioDestinationNode()>("getDestination");
        auto destination = method(javaObject_.get());
        auto destinationCppInstance = destination->cthis();

        return std::shared_ptr<AudioDestinationNode>(destinationCppInstance);
    }

    std::shared_ptr<GainNode> AudioContext::createGain()
    {
        static const auto method = javaClassLocal()->getMethod<GainNode()>("createGain");
        auto gain = method(javaObject_.get());
        auto gainCppInstance = gain->cthis();

        return std::shared_ptr<GainNode>(gainCppInstance);
    }

    std::shared_ptr<StereoPannerNode> AudioContext::createStereoPanner()
    {
        static const auto method = javaClassLocal()->getMethod<StereoPannerNode()>("createStereoPanner");
        auto stereoPanner = method(javaObject_.get());
        auto stereoPannerCppInstance = stereoPanner->cthis();

        return std::shared_ptr<StereoPannerNode>(stereoPannerCppInstance);
    }

    std::string AudioContext::getState()
    {
        static const auto method = javaClassLocal()->getMethod<JString()>("getState");
        return method(javaObject_.get())->toStdString();
    }

    int AudioContext::getSampleRate()
    {
        static const auto method = javaClassLocal()->getMethod<jint()>("getSampleRate");
        return method(javaObject_.get());
    }

    double AudioContext::getCurrentTime()
    {
        static const auto method = javaClassLocal()->getMethod<jdouble()>("getCurrentTime");
        return method(javaObject_.get());
    }
} // namespace audiocontext
