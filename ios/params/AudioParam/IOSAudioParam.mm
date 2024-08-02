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

} // namespace audiocontext
