#include <IOSAudioParam.h>

namespace audiocontext {

    void IOSAudioParam::setValue(float value) {
        [audioParam setValue:value];
    }

    float IOSAudioParam::getValue() {
        return [audioParam getValue];
    }

    float IOSAudioParam::getMinValue() {
        return [audioParam getMinValue];
    }

    float IOSAudioParam::getMaxValue() {
        return [audioParam getMaxValue];
    }

    float IOSAudioParam::getDefaultValue() {
        return [audioParam getDefaultValue];
    }

    void IOSAudioParam::setValueAtTime(float value, double time) {
        [audioParam setValueAtTime:value time:time];
    }

    void IOSAudioParam::linearRampToValueAtTime(float value, double time) {
        [audioParam linearRampToValueAtTime:value time:time];
    }

    void IOSAudioParam::exponentialRampToValueAtTime(float value, double time) {
        [audioParam exponentialRampToValueAtTime:value time:time];
    }

} // namespace audiocontext
