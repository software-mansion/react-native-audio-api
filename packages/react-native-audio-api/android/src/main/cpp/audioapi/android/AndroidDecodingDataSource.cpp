#include <audioapi/android/AndroidDecodingDataSource.h>

#include <media/NdkMediaError.h>
#include <media/NdkMediaExtractor.h>

#include <dlfcn.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

// AMediaDataSource / setDataSourceCustom require API 28. To support lower API
// levels at build/link time while using these on newer devices, we load them
// dynamically from libmediandk.so.
#if __has_include(<media/NdkMediaDataSource.h>)
#include <media/NdkMediaDataSource.h>
#else
// Fallback definitions if header is missing (unlikely given it compiled).
struct AMediaDataSource;
typedef ssize_t (
    *AMediaDataSourceReadAt)(void *userdata, off64_t offset, void *buffer, size_t size);
typedef ssize_t (*AMediaDataSourceGetSize)(void *userdata);
typedef void (*AMediaDataSourceClose)(void *userdata);
#endif

namespace audioapi::android_decoder {

namespace {

// Function pointer types for dynamic loading.
typedef AMediaDataSource *(*pfn_AMediaDataSource_new)();
typedef void (*pfn_AMediaDataSource_delete)(AMediaDataSource *);
typedef void (*pfn_AMediaDataSource_setUserdata)(AMediaDataSource *, void *);
typedef void (*pfn_AMediaDataSource_setReadAt)(AMediaDataSource *, AMediaDataSourceReadAt);
typedef void (*pfn_AMediaDataSource_setGetSize)(AMediaDataSource *, AMediaDataSourceGetSize);
typedef void (*pfn_AMediaDataSource_setClose)(AMediaDataSource *, AMediaDataSourceClose);
typedef media_status_t (
    *pfn_AMediaExtractor_setDataSourceCustom)(AMediaExtractor *, AMediaDataSource *);

struct DynamicNDK {
  pfn_AMediaDataSource_new AMediaDataSource_new = nullptr;
  pfn_AMediaDataSource_delete AMediaDataSource_delete = nullptr;
  pfn_AMediaDataSource_setUserdata AMediaDataSource_setUserdata = nullptr;
  pfn_AMediaDataSource_setReadAt AMediaDataSource_setReadAt = nullptr;
  pfn_AMediaDataSource_setGetSize AMediaDataSource_setGetSize = nullptr;
  pfn_AMediaDataSource_setClose AMediaDataSource_setClose = nullptr;
  pfn_AMediaExtractor_setDataSourceCustom AMediaExtractor_setDataSourceCustom = nullptr;

  bool initialized = false;

  static DynamicNDK &get() {
    static DynamicNDK instance;
    if (!instance.initialized) {
      void *lib = dlopen("libmediandk.so", RTLD_NOW);
      if (lib) {
        instance.AMediaDataSource_new =
            (pfn_AMediaDataSource_new)dlsym(lib, "AMediaDataSource_new");
        instance.AMediaDataSource_delete =
            (pfn_AMediaDataSource_delete)dlsym(lib, "AMediaDataSource_delete");
        instance.AMediaDataSource_setUserdata =
            (pfn_AMediaDataSource_setUserdata)dlsym(lib, "AMediaDataSource_setUserdata");
        instance.AMediaDataSource_setReadAt =
            (pfn_AMediaDataSource_setReadAt)dlsym(lib, "AMediaDataSource_setReadAt");
        instance.AMediaDataSource_setGetSize =
            (pfn_AMediaDataSource_setGetSize)dlsym(lib, "AMediaDataSource_setGetSize");
        instance.AMediaDataSource_setClose =
            (pfn_AMediaDataSource_setClose)dlsym(lib, "AMediaDataSource_setClose");
        instance.AMediaExtractor_setDataSourceCustom =
            (pfn_AMediaExtractor_setDataSourceCustom)dlsym(
                lib, "AMediaExtractor_setDataSourceCustom");
      }
      instance.initialized = true;
    }
    return instance;
  }

  bool isAvailable() const {
    return AMediaDataSource_new != nullptr && AMediaDataSource_delete != nullptr &&
        AMediaExtractor_setDataSourceCustom != nullptr;
  }
};

ssize_t memoryDataSourceReadAt(void *userdata, off64_t offset, void *buffer, size_t size) {
  auto *context = static_cast<MemoryDataSourceContext *>(userdata);
  if (context == nullptr || context->data == nullptr || offset < 0 ||
      static_cast<size_t>(offset) >= context->size) {
    return -1;
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
  if (handle_ != nullptr) {
    auto &ndk = DynamicNDK::get();
    if (ndk.AMediaDataSource_delete) {
      ndk.AMediaDataSource_delete(static_cast<AMediaDataSource *>(handle_));
    }
    handle_ = nullptr;
  }
}

decoding::DecoderResult attachMemoryExtractorViaDataSource(
    AMediaExtractor *extractor,
    AndroidMemoryDataSource &dataSource,
    std::vector<uint8_t> &encodedMemory,
    MemoryDataSourceContext &memorySourceContext,
    std::vector<uint8_t> data) {

  auto &ndk = DynamicNDK::get();
  if (!ndk.isAvailable()) {
    return Err(
        "AndroidDecoder: AMediaDataSource is not supported on this device (requires API 28)");
  }

  encodedMemory = std::move(data);
  memorySourceContext = {encodedMemory.data(), encodedMemory.size()};

  AMediaDataSource *rawDataSource = ndk.AMediaDataSource_new();
  if (rawDataSource == nullptr) {
    return Err("AndroidDecoder::open: AMediaDataSource_new failed");
  }
  ndk.AMediaDataSource_setUserdata(rawDataSource, &memorySourceContext);
  ndk.AMediaDataSource_setReadAt(rawDataSource, memoryDataSourceReadAt);
  ndk.AMediaDataSource_setGetSize(rawDataSource, memoryDataSourceGetSize);
  ndk.AMediaDataSource_setClose(rawDataSource, memoryDataSourceClose);

  if (ndk.AMediaExtractor_setDataSourceCustom(extractor, rawDataSource) != AMEDIA_OK) {
    ndk.AMediaDataSource_delete(rawDataSource);
    return Err("AndroidDecoder::open setDataSourceCustom failed");
  }

  dataSource.reset();
  dataSource.handle_ = rawDataSource;
  return Ok(None);
}

} // namespace audioapi::android_decoder
