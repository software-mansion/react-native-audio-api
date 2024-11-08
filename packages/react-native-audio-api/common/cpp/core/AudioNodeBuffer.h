#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// namespace audioapi {

// class AudioNodeBuffer {
//   public:
//     explicit AudioNodeBuffer(BaseAudioContext * context, int channelCount);

//     void zero();
//     void normalize();
//     void scale(float scale);

//     void sumFrom(const AudioNodeBuffer &source);
//     void copyFrom(const AudioNodeBuffer &source);

//     float maxAbsValue() const;

//   private:
//     BaseAudioContext *context_;
//     std::unique_ptr<AudioBuffer> buffer_;

//     int bufferSizeInFrames_;
//     int channelCount_;
// };

// }
