#pragma once

#include <memory>

#ifndef __OBJC__
typedef struct AudioBufferList AudioBufferList;
#endif

namespace audioapi::ios {

/// Frees an AudioBufferList allocated by `allocateOwnedAudioBufferList`.
void freeOwnedAudioBufferList(const AudioBufferList *bufferList);

/// unique_ptr deleter that releases an owned AudioBufferList and its channel buffers.
struct OwnedAudioBufferListDeleter {
  void operator()(AudioBufferList *bufferList) const noexcept {
    freeOwnedAudioBufferList(bufferList);
  }
};

/// Owning handle for an AudioBufferList from `allocateOwnedAudioBufferList`.
using OwnedAudioBufferListPtr = std::unique_ptr<AudioBufferList, OwnedAudioBufferListDeleter>;

/// Allocates an AudioBufferList with fixed per-buffer capacity.
/// Returns nullptr on allocation failure. Caller must free with
/// `freeOwnedAudioBufferList`.
AudioBufferList *allocateOwnedAudioBufferList(
    unsigned int bufferCount,
    unsigned int bytesPerBuffer);

/// Copies `source` into `destination` without allocating.
/// `maxBytesPerBuffer` is the fixed per-buffer capacity from `allocateOwnedAudioBufferList`.
/// Returns false when `source` does not fit in that capacity.
bool copyIntoOwnedAudioBufferList(
    AudioBufferList *destination,
    const AudioBufferList *source,
    unsigned int maxBytesPerBuffer);

/// Deep-copies a Core Audio buffer list so it can outlive a synchronous I/O callback.
/// Returns nullptr on allocation failure. Caller must free with `freeOwnedAudioBufferList`.
AudioBufferList *copyAudioBufferList(const AudioBufferList *audioBufferList);

} // namespace audioapi::ios
