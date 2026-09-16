#pragma once

#include <audioapi/decoding/backends/AudioDecoderBackend.h>

#include <cstddef>
#include <cstdint>
#include <vector>

struct AMediaExtractor;

namespace audioapi::android_decoder {

// AMediaDataSource / setDataSourceCustom require API 28. minSdk may be lower, so
// AndroidDecodingDataSource.cpp weak-links these symbols and guards every call;
// below this level callers must use the temp-file fallback instead.
constexpr int kMediaDataSourceMinApiLevel = 28;

struct MemoryDataSourceContext {
  const uint8_t *data = nullptr;
  size_t size = 0;
};

/// Opaque owning handle for AMediaDataSource (kept as void* so NDK media types
/// stay out of cross-platform includes).
class AndroidMemoryDataSource {
 public:
  AndroidMemoryDataSource() = default;
  ~AndroidMemoryDataSource();
  AndroidMemoryDataSource(AndroidMemoryDataSource &&other) noexcept;
  AndroidMemoryDataSource &operator=(AndroidMemoryDataSource &&other) noexcept;
  AndroidMemoryDataSource(const AndroidMemoryDataSource &) = delete;
  AndroidMemoryDataSource &operator=(const AndroidMemoryDataSource &) = delete;

  void reset();

 private:
  friend decoding::DecoderResult attachMemoryExtractorViaDataSource(
      AMediaExtractor *extractor,
      AndroidMemoryDataSource &dataSource,
      std::vector<uint8_t> &encodedMemory,
      MemoryDataSourceContext &memorySourceContext,
      std::vector<uint8_t> data);

  void *handle_ = nullptr;
};

[[nodiscard]] decoding::DecoderResult attachMemoryExtractorViaDataSource(
    AMediaExtractor *extractor,
    AndroidMemoryDataSource &dataSource,
    std::vector<uint8_t> &encodedMemory,
    MemoryDataSourceContext &memorySourceContext,
    std::vector<uint8_t> data);

} // namespace audioapi::android_decoder
