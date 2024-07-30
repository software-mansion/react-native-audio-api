#include <IOSAudioParam.h>

namespace audiocontext {

    void IOSAudioParam::setValue(float value) {
        [audioParam setValue:value];
    }

    float IOSAudioParam::getValue() {
        return [audioParam getValue];
    }

} // namespace audiocontext
