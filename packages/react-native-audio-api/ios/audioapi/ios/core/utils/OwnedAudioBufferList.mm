#import <CoreAudio/CoreAudioTypes.h>

#include <audioapi/ios/core/utils/OwnedAudioBufferList.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace audioapi::ios {

void freeOwnedAudioBufferList(const AudioBufferList *bufferList)
{
  if (bufferList == nullptr) {
    return;
  }
  for (UInt32 i = 0; i < bufferList->mNumberBuffers; ++i) {
    std::free(bufferList->mBuffers[i].mData);
  }
  std::free(const_cast<AudioBufferList *>(bufferList));
}

AudioBufferList *copyAudioBufferList(const AudioBufferList *audioBufferList)
{
  if (audioBufferList == nullptr) {
    return nullptr;
  }

  UInt32 bufferCount = audioBufferList->mNumberBuffers;
  size_t headerSize = offsetof(AudioBufferList, mBuffers) + sizeof(AudioBuffer) * bufferCount;
  auto *owned = static_cast<AudioBufferList *>(std::malloc(headerSize));
  if (owned == nullptr) {
    return nullptr;
  }
  owned->mNumberBuffers = bufferCount;
  for (UInt32 i = 0; i < bufferCount; ++i) {
    UInt32 byteSize = audioBufferList->mBuffers[i].mDataByteSize;
    owned->mBuffers[i].mNumberChannels = audioBufferList->mBuffers[i].mNumberChannels;
    owned->mBuffers[i].mDataByteSize = byteSize;
    void *channelData = std::malloc(byteSize);
    if (channelData == nullptr) {
      for (UInt32 j = 0; j < i; ++j) {
        std::free(owned->mBuffers[j].mData);
      }
      std::free(owned);
      return nullptr;
    }
    std::memcpy(channelData, audioBufferList->mBuffers[i].mData, byteSize);
    owned->mBuffers[i].mData = channelData;
  }
  return owned;
}

} // namespace audioapi::ios
