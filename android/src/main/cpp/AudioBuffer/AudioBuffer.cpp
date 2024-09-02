#include "AudioBuffer.h"

namespace audiocontext {

    AudioBuffer::AudioBuffer(jni::alias_ref<AudioBuffer::jhybridobject>& jThis): javaPart_(make_global(jThis)) {}

    int AudioBuffer::getSampleRate() {
        static const auto method = javaClassStatic()->getMethod<jint()>("getSampleRate");
        return method(javaPart_);
    }

    int AudioBuffer::getLength() {
        static const auto method = javaClassStatic()->getMethod<jint()>("getLength");
        return method(javaPart_);
    }

    double AudioBuffer::getDuration() {
        static const auto method = javaClassStatic()->getMethod<jdouble()>("getDuration");
        return method(javaPart_);
    }

    int AudioBuffer::getNumberOfChannels() {
        static const auto method = javaClassStatic()->getMethod<jint()>("getNumberOfChannels");
        return method(javaPart_);
    }

    short **AudioBuffer::getChannelData(int channel) {
        static const auto method = javaClassStatic()->getMethod<JArrayShort (jint)>("getChannelData");
        auto jArray = method(javaPart_, channel);
        auto length = jArray->size();

        auto channelData = new short*[length];
        for (int i = 0; i < length; i++) {
            channelData[i] = &jArray->pin()[i];
        }

        return channelData;
    }

    void AudioBuffer::prepareForDeconstruction() {
        javaPart_.reset();
    }

} // namespace audiocontext
