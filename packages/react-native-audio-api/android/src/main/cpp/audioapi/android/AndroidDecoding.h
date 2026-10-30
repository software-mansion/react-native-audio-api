#pragma once

#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/OsDecoderBase.h>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <memory>

namespace audioapi::android::decoder {

struct AndroidDecoderState;

/**
 * Android MediaExtractor + MediaCodec incremental decoder.
 *
 * Opens local files or in-memory buffers and pulls interleaved float PCM on
 * demand. Container metadata (including duration) is read at open time; the
 * MediaCodec pipeline starts lazily on the first read or seek, in async
 * (callback) mode on API 28+ and sync mode below. In-memory
 * sources use AMediaDataSource on API 28+ (runtime check); API 21–27 falls
 * back to a temp file + fd. Remote URL / HLS decoding stays on FFmpeg.
 */
class AndroidDecoder : public decoding::OsDecoderBase {
 public:
  AndroidDecoder();
  ~AndroidDecoder() override;
  DELETE_COPY_AND_MOVE(AndroidDecoder);

  [[nodiscard]] decoding::DecoderResult open(const decoding::LocalFileSource &source);
  [[nodiscard]] decoding::DecoderResult open(const decoding::EncodedMemorySource &source);
  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;
  [[nodiscard]] decoding::DecoderResult seekToTime(double seconds) override;
  /// Boosts CPU clocks for the decoding threads through an ADPF performance
  /// hint session on API 33+; does nothing below that.
  void preferThroughput() override;

 private:
  [[nodiscard]] decoding::DecoderResult finishOpeningDecoder(
      std::unique_ptr<AndroidDecoderState> state,
      int sampleRate);

  void releaseImpl() override;

  // Opaque NDK state (defined in AndroidDecoding.cpp).
  std::unique_ptr<AndroidDecoderState> impl_;
  bool preferThroughput_ = false;
};

} // namespace audioapi::android::decoder
