#include <audioapi/android/AndroidDecodingDataSource.h>

#include <media/NdkMediaDataSource.h>
#include <media/NdkMediaError.h>
#include <media/NdkMediaExtractor.h>

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

// AMediaDataSource / setDataSourceCustom require API 28 while minSdk is lower.
// This translation unit is compiled with __ANDROID_UNAVAILABLE_SYMBOLS_ARE_WEAK__
// (see CMakeLists.txt), so these symbols become weak imports that resolve to
// nullptr on older devices instead of failing library load; every use is guarded
// with __builtin_available. Requires NDK r26+.

namespace audioapi::android_decoder {

namespace {

ssize_t memoryDataSourceReadAt(void *userdata, off64_t offset, void *buffer, size_t size) {
  auto *context = static_cast<MemoryDataSourceContext *>(userdata);
  if (context == nullptr || context->data == nullptr || offset < 0) {
    return -1;
  }
  if (static_cast<size_t>(offset) >= context->size) {
    return 0;
  }
  const size_t available = context->size - static_cast<size_t>(offset);
  const size_t toCopy = std::min(size, available);
  if (toCopy > 0) {
    std::memcpy(buffer, context->data + offset, toCopy);
  }
  return static_cast<ssize_t>(toCopy);
}

ssize_t memoryDataSourceGetSize(void *userdata) {
  auto *context = static_cast<MemoryDataSourceContext *>(userdata);
  if (context == nullptr) {
    return -1;
  }
  return static_cast<ssize_t>(context->size);
}

void memoryDataSourceClose(void * /*userdata*/) {}

} // namespace

AndroidMemoryDataSource::~AndroidMemoryDataSource() {
  reset();
}

AndroidMemoryDataSource::AndroidMemoryDataSource(AndroidMemoryDataSource &&other) noexcept
    : handle_(other.handle_) {
  other.handle_ = nullptr;
}

AndroidMemoryDataSource &AndroidMemoryDataSource::operator=(
    AndroidMemoryDataSource &&other) noexcept {
  if (this != &other) {
    reset();
    handle_ = other.handle_;
    other.handle_ = nullptr;
  }
  return *this;
}

void AndroidMemoryDataSource::reset() {
  if (handle_ == nullptr) {
    return;
  }
  if (__builtin_available(android 28, *)) {
    AMediaDataSource_delete(static_cast<AMediaDataSource *>(handle_));
  }
  handle_ = nullptr;
}

decoding::DecoderResult attachMemoryExtractorViaDataSource(
    AMediaExtractor *extractor,
    AndroidMemoryDataSource &dataSource,
    std::vector<uint8_t> &encodedMemory,
    MemoryDataSourceContext &memorySourceContext,
    std::vector<uint8_t> data) {
  if (__builtin_available(android 28, *)) {
    encodedMemory = std::move(data);
    memorySourceContext = {encodedMemory.data(), encodedMemory.size()};

    AMediaDataSource *rawDataSource = AMediaDataSource_new();
    if (rawDataSource == nullptr) {
      return Err("AndroidDecoder::open: AMediaDataSource_new failed");
    }
    AMediaDataSource_setUserdata(rawDataSource, &memorySourceContext);
    AMediaDataSource_setReadAt(rawDataSource, memoryDataSourceReadAt);
    AMediaDataSource_setGetSize(rawDataSource, memoryDataSourceGetSize);
    AMediaDataSource_setClose(rawDataSource, memoryDataSourceClose);

    if (AMediaExtractor_setDataSourceCustom(extractor, rawDataSource) != AMEDIA_OK) {
      AMediaDataSource_delete(rawDataSource);
      return Err("AndroidDecoder::open setDataSourceCustom failed");
    }

    dataSource.reset();
    dataSource.handle_ = rawDataSource;
    return Ok(None);
  }

  return Err("AndroidDecoder: AMediaDataSource is not supported on this device (requires API 28)");
}

} // namespace audioapi::android_decoder
