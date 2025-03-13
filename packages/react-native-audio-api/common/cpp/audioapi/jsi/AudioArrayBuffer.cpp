#include <audioapi/jsi/AudioArrayBuffer.h>

namespace audioapi {

size_t AudioArrayBuffer::size() const {
  return size_;
}

uint8_t *AudioArrayBuffer::data() {
  return data_;
}

} // namespace audioapi
