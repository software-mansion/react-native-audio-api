#include "AudioContext.h"

namespace audiocontext
{

  using namespace facebook::jni;

    AudioContext::AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis) : javaPart_(make_global(jThis)) {};

    void AudioContext::install(jlong jsContext)
    {
        auto audioContextWrapper = std::make_shared<AudioContextWrapper>(this);
        AudioContextHostObject::createAndInstallFromWrapper(audioContextWrapper, jsContext);
    }

    AudioDestinationNode* AudioContext::getDestination()
    {
        static const auto method = javaClassLocal()->getMethod<AudioDestinationNode()>("getDestination");
        auto destination = method(javaPart_.get());

       return destination->cthis();
    }

    OscillatorNode* AudioContext::createOscillator()
    {
        static const auto method = javaClassLocal()->getMethod<OscillatorNode()>("createOscillator");
        auto oscillator = method(javaPart_.get());

        return oscillator->cthis();
    }

    GainNode* AudioContext::createGain()
    {
        static const auto method = javaClassLocal()->getMethod<GainNode()>("createGain");
        auto gain = method(javaPart_.get());

        return gain->cthis();
    }

    StereoPannerNode* AudioContext::createStereoPanner()
    {
        static const auto method = javaClassLocal()->getMethod<StereoPannerNode()>("createStereoPanner");
        auto stereoPanner = method(javaPart_.get());

        return stereoPanner->cthis();
    }

    BiquadFilterNode* AudioContext::createBiquadFilter()
    {
        static const auto method = javaClassLocal()->getMethod<BiquadFilterNode()>("createBiquadFilter");
        auto biquadFilter = method(javaPart_.get());

        return biquadFilter->cthis();
    }

    std::string AudioContext::getState()
    {
        static const auto method = javaClassLocal()->getMethod<JString()>("getState");
        return method(javaPart_.get())->toStdString();
    }

    int AudioContext::getSampleRate()
    {
        static const auto method = javaClassLocal()->getMethod<jint()>("getSampleRate");
        return method(javaPart_.get());
    }

    double AudioContext::getCurrentTime()
    {
        static const auto method = javaClassLocal()->getMethod<jdouble()>("getCurrentTime");
        return method(javaPart_.get());
    }

    void AudioContext::close()
    {
        static const auto method = javaClassLocal()->getMethod<void()>("close");
        method(javaPart_.get());
        javaPart_.reset();
    }
} // namespace audiocontext
