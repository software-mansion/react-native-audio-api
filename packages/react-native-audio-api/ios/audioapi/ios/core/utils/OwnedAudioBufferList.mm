#import <CoreAudio/CoreAudioTypes.h>

#include <audioapi/ios/core/utils/OwnedAudioBufferList.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>

namespace audioapi::ios {

AudioBufferList *allocateOwnedAudioBufferList(unsigned int bufferCount, unsigned int bytesPerBuffer)
{
  size_t headerSize = offsetof(AudioBufferList, mBuffers) + sizeof(AudioBuffer) * bufferCount;
  auto *owned = static_cast<AudioBufferList *>(std::malloc(headerSize));
  if (owned == nullptr) {
    return nullptr;
  }

  owned->mNumberBuffers = bufferCount;
  for (UInt32 i = 0; i < bufferCount; ++i) {
    owned->mBuffers[i].mNumberChannels = 0;
    owned->mBuffers[i].mDataByteSize = bytesPerBuffer;
    owned->mBuffers[i].mData = std::malloc(bytesPerBuffer);
    if (owned->mBuffers[i].mData == nullptr) {
      for (UInt32 j = 0; j < i; ++j) {
        std::free(owned->mBuffers[j].mData);
      }
      std::free(owned);
      return nullptr;
    }
  }

  return owned;
}

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

bool copyIntoOwnedAudioBufferList(
    AudioBufferList *destination,
    const AudioBufferList *source,
    unsigned int maxBytesPerBuffer)
{
  if (destination == nullptr || source == nullptr) {
    return false;
  }

  if (destination->mNumberBuffers != source->mNumberBuffers) {
    return false;
  }

  for (UInt32 i = 0; i < source->mNumberBuffers; ++i) {
    auto srcByteSize = source->mBuffers[i].mDataByteSize;
    if (srcByteSize > maxBytesPerBuffer) {
      return false;
    }
    if (destination->mBuffers[i].mData == nullptr) {
      return false;
    }
    if (srcByteSize > 0 && source->mBuffers[i].mData == nullptr) {
      return false;
    }

    std::memcpy(destination->mBuffers[i].mData, source->mBuffers[i].mData, srcByteSize);
    destination->mBuffers[i].mNumberChannels = source->mBuffers[i].mNumberChannels;
    destination->mBuffers[i].mDataByteSize = srcByteSize;
  }

  return true;
}

AudioBufferList *copyAudioBufferList(const AudioBufferList *audioBufferList)
{
  if (audioBufferList == nullptr) {
    return nullptr;
  }

  UInt32 bufferCount = audioBufferList->mNumberBuffers;
  UInt32 bytesPerBuffer = bufferCount > 0 ? audioBufferList->mBuffers[0].mDataByteSize : 0;

  auto *owned = allocateOwnedAudioBufferList(bufferCount, bytesPerBuffer);
  if (owned == nullptr) {
    return nullptr;
  }

  for (UInt32 i = 0; i < bufferCount; ++i) {
    if (audioBufferList->mBuffers[i].mDataByteSize > bytesPerBuffer) {
      freeOwnedAudioBufferList(owned);
      owned = allocateOwnedAudioBufferList(bufferCount, audioBufferList->mBuffers[i].mDataByteSize);
      if (owned == nullptr) {
        return nullptr;
      }
      bytesPerBuffer = audioBufferList->mBuffers[i].mDataByteSize;
    }
  }

  if (!copyIntoOwnedAudioBufferList(owned, audioBufferList, bytesPerBuffer)) {
    freeOwnedAudioBufferList(owned);
    return nullptr;
  }

  return owned;
}

} // namespace audioapi::ios
