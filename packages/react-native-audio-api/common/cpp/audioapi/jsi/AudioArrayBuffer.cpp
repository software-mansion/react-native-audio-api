#include <audioapi/jsi/AudioArrayBuffer.h>

namespace audioapi {

size_t AudioArrayBuffer::size() const {
  return size_;
}

uint8_t *AudioArrayBuffer::data() {
  return const_cast<uint8_t *>(data_);
}

} // namespace audioapi
