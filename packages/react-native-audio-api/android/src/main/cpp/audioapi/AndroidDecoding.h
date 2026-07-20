#pragma once

#include <audioapi/decoding/OsDecoderBase.h>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <map>
#include <memory>
#include <string>

namespace audioapi::android_decoder {

struct AndroidDecoderState;

/**
 * Android MediaExtractor + MediaCodec incremental decoder.
 *
 * Opens local files or in-memory buffers and pulls interleaved float PCM on
 * demand. `openUrl` / HLS are not supported (callers keep those on FFmpeg).
 */
class AndroidDecoder : public OsDecoderBase {
 public:
  AndroidDecoder();
  ~AndroidDecoder() override;
  DELETE_COPY_AND_MOVE(AndroidDecoder);

  [[nodiscard]] decoding::DecoderResult openFile(int outputSampleRate, const std::string &path)
      override;
  [[nodiscard]] decoding::DecoderResult
  openMemory(int outputSampleRate, const void *data, size_t size) override;
  [[nodiscard]] decoding::DecoderResult openUrl(
      int outputSampleRate,
      const std::string &url,
      const std::map<std::string, std::string> &headers = {}) override;
  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;
  [[nodiscard]] decoding::DecoderResult seekToTime(double seconds) override;

 private:
  void releaseImpl() override;

  // Opaque NDK state (defined in AndroidDecoding.cpp).
  std::unique_ptr<AndroidDecoderState> impl_;
};

} // namespace audioapi::android_decoder
