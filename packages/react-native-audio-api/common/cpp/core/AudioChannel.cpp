#include "AudioChannel.h"
#include "AudioArray.h"

namespace audioapi {

explicit AudioChannel::AudioChannel(int length, int sampleRate) : length_(length), sampleRate_(sampleRate) {
  data_ = std::make_unique<AudioArray>(length_);
};

int AudioChannel::getLength() const {
  return length_;
};

int AudioChannel::getSampleRate() const {
  return sampleRate_;
};

} // namespace audioapi
