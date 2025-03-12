#include <audioapi/jsi/AudioArrayBuffer.h>

namespace audioapi {

AudioArrayBuffer::AudioArrayBuffer(float *data, size_t size)
    : data_(data), size_(size) {}

size_t AudioArrayBuffer::size() const {
  return size_;
}

uint8_t *AudioArrayBuffer::data() {
  return reinterpret_cast<uint8_t *>(data_);
}

} // namespace audioapi
