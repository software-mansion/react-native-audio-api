#pragma once

#ifndef __OBJC__
typedef struct AudioBufferList AudioBufferList;
#endif

namespace audioapi::ios {

/// Frees an AudioBufferList allocated by `copyAudioBufferList`.
void freeOwnedAudioBufferList(const AudioBufferList *bufferList);

/// Deep-copies a Core Audio buffer list so it can outlive a synchronous I/O callback.
/// Returns nullptr on allocation failure. Caller must free with `freeOwnedAudioBufferList`.
AudioBufferList *copyAudioBufferList(const AudioBufferList *audioBufferList);

} // namespace audioapi::ios
