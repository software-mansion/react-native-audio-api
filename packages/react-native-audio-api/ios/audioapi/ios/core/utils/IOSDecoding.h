#pragma once

#include <audioapi/decoding/DecoderSource.h>
#include <audioapi/decoding/backends/OsDecoderBase.h>
#include <audioapi/utils/Macros.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace audioapi::ios_decoder {

struct IosDecoderState;

/**
 * iOS ExtAudioFile-backed incremental decoder.
 *
 * Opens local files or in-memory buffers and pulls interleaved float PCM on
 * demand. Remote URL / HLS decoding stays on FFmpeg.
 */
class IOSDecoder : public decoding::OsDecoderBase {
 public:
  IOSDecoder();
  ~IOSDecoder() override;
  DELETE_COPY_AND_MOVE(IOSDecoder);

  [[nodiscard]] decoding::DecoderResult open(const decoding::LocalFileSource &source);
  [[nodiscard]] decoding::DecoderResult open(const decoding::EncodedMemorySource &source);
  [[nodiscard]] size_t readPcmFrames(float *outInterleaved, size_t frameCount) override;
  [[nodiscard]] decoding::DecoderResult seekToTime(double seconds) override;

 private:
  [[nodiscard]] decoding::DecoderResult finishOpeningDecoder(
      std::unique_ptr<IosDecoderState> state,
      int requestedSampleRate);

  void releaseImpl() override;

  // Opaque Core Audio state (defined in IOSDecoding.mm).
  std::unique_ptr<IosDecoderState> impl_;
};

} // namespace audioapi::ios_decoder
