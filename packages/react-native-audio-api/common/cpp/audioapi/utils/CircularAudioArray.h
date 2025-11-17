#pragma once

#include <audioapi/utils/AudioArray.h>
#include <stdexcept>

namespace audioapi {

class CircularAudioArray : public AudioArray {
 public:
  explicit CircularAudioArray(size_t size);
  CircularAudioArray(const CircularAudioArray &other) = default;
  ~CircularAudioArray() = default;

  void push_back(const float *data, size_t size, bool skipAvailableSpaceCheck = false);
  void pop_front(float *data, size_t size, bool skipAvailableDataCheck = false);
  void pop_back(float *data, size_t size, size_t offset = 0, bool skipAvailableDataCheck = false);

  [[nodiscard]] size_t getNumberOfAvailableFrames() const;

 private:
  size_t vWriteIndex_ = 0;
  size_t vReadIndex_ = 0;

  [[nodiscard]] size_t getAvailableSpace() const;
};

} // namespace audioapi
