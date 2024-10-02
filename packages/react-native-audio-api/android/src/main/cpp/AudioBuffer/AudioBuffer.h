#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

// TODO implement AudioBuffer class

class AudioBuffer {
public:
    explicit AudioBuffer(int numberOfChannels, int length, int sampleRate);

private:
    int numberOfChannels_;
    int length_;
    int sampleRate_;
};

} // namespace audioapi
