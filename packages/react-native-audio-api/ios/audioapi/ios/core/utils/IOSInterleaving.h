#pragma once

#import <AudioToolbox/AudioToolbox.h>

#include <vector>

namespace audioapi::ios_interleaving {

/// @brief Repacks a CoreAudio input buffer as interleaved float32, the format every
/// consumer downstream expects. Returns nullptr — dropping the buffer — when the layout no
/// longer matches what we were configured with, which happens if the input node is rebuilt
/// on a route change while a recording is in flight.
/// @param interleavedHolder Scratch buffer the planar case packs into; must already hold at
/// least numFrames * channelCount samples. Unused when the input is already interleaved.
[[nodiscard]] const float *interleaveAudioInput(
    const AudioBufferList *input,
    int numFrames,
    int channelCount,
    std::vector<float> &interleavedHolder);

} // namespace audioapi::ios_interleaving
