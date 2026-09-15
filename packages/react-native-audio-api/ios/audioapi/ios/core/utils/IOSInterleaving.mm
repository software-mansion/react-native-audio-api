#import <AudioToolbox/AudioToolbox.h>

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/ios/core/utils/IOSInterleaving.h>

#include <cstddef>
#include <vector>

namespace audioapi::ios_interleaving {

const float *interleaveAudioInput(
    const AudioBufferList *input,
    int numFrames,
    int channelCount,
    std::vector<float> &interleavedHolder)
{
  if (input == nullptr || numFrames <= 0 || channelCount <= 0 || channelCount > MAX_CHANNEL_COUNT) {
    return nullptr;
  }

  const auto frames = static_cast<size_t>(numFrames);
  const size_t samples = frames * static_cast<size_t>(channelCount);

  // Mono, or an already-interleaved format: hand CoreAudio's own buffer straight through.
  if (input->mNumberBuffers == 1) {
    if (input->mBuffers[0].mDataByteSize < samples * sizeof(float)) {
      return nullptr;
    }
    return static_cast<const float *>(input->mBuffers[0].mData);
  }

  if (input->mNumberBuffers != static_cast<UInt32>(channelCount) ||
      samples > interleavedHolder.size()) {
    return nullptr;
  }

  const float *channelPointers[MAX_CHANNEL_COUNT];
  for (int channel = 0; channel < channelCount; ++channel) {
    if (input->mBuffers[channel].mDataByteSize < frames * sizeof(float)) {
      return nullptr;
    }
    channelPointers[channel] = static_cast<const float *>(input->mBuffers[channel].mData);
  }

  dsp::interleave(
      channelPointers, static_cast<size_t>(channelCount), interleavedHolder.data(), frames);
  return interleavedHolder.data();
}

} // namespace audioapi::ios_interleaving
