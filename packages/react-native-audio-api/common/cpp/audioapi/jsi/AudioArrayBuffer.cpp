#include <audioapi/jsi/AudioArrayBuffer.h>

namespace audioapi {

size_t AudioArrayBuffer::size() const {
  return audioArray_->getSize() * sizeof(float);
}

uint8_t *AudioArrayBuffer::data() {
  return reinterpret_cast<uint8_t *>(audioArray_->getData());
}

} // namespace audioapi
