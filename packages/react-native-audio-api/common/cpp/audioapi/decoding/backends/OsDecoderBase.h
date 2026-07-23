#pragma once

#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <audioapi/utils/Macros.h>

namespace audioapi::decoding {

/**
 * Shared open/close for OS-native incremental decoders (iOS / Android).
 *
 * Metadata lives on `AudioDecoderBackend`; subclasses release platform
 * resources in `releaseImpl()` (e.g. `impl_.reset()`).
 */
class OsDecoderBase : public AudioDecoderBackend {
 public:
  OsDecoderBase() = default;
  ~OsDecoderBase() override = default;
  DELETE_COPY_AND_MOVE(OsDecoderBase);

  void close() final {
    releaseImpl();
    resetOpenMetadata();
    open_ = false;
  }

  [[nodiscard]] bool isOpen() const final {
    return open_;
  }

 protected:
  /// Drop platform-native decoder resources (ExtAudioFile / MediaCodec, etc.).
  virtual void releaseImpl() = 0;

  bool open_ = false;
};

} // namespace audioapi::decoding
