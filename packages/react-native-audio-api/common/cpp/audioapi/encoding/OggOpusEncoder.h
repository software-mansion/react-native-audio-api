#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/encoding/BundledEncoders.h>
#include <audioapi/utils/Result.hpp>

#include <cstddef>
#include <memory>
#include <string>

#if RN_AUDIO_API_BUNDLED_OGG_OPUS

namespace audioapi::ogg_opus {

struct EncoderState;

/// Opus in an Ogg container (RFC 7845) through the bundled libopus and libogg, for platforms
/// whose system encoders cannot write that pair. open()/close() run on the JS thread and
/// encode() on the file-writer worker; nothing here touches the audio thread.
///
/// Opus codes at 48 kHz only, so the file is 48 kHz whatever `fileLayout.sampleRate` asks for:
/// the input is channel-mapped to `fileLayout.channelCount` (mono or stereo) and resampled.
/// Packets hold 20 ms each, at `encoding.bitRate` (96 kbps when unset).
class OggOpusEncoder final : public AudioEncoder {
 public:
  explicit OggOpusEncoder(const EncoderSettings &settings);
  ~OggOpusEncoder() override;

  /// Writes the Ogg stream headers at once, so the file is non-empty from here on. Refuses
  /// any output spec other than Opus in Ogg.
  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec &outputSpec,
      const std::string &filePath) override;

  EncodeResult encode(const float *const *channels, int numFrames) override;

  /// Pads the last packet with silence and trims it back through the end-of-stream granule
  /// position, so a decoder returns exactly the frames that were encoded.
  CloseEncoderResult close() override;

  /// Bytes written so far, without a stat(): the Ogg pages go straight to the file.
  [[nodiscard]] size_t getFileSizeBytes() const override;

 protected:
  /// Rebuilds the channel map and the resampler for the new input; the Ogg stream and the Opus
  /// encoder carry on in the same file. The frames the old resampler still held, at most its
  /// group delay, go with it.
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) override;

 private:
  Result<NoneType, std::string> prepareConversion(const StreamFormat &inputFormat);
  /// Maps and resamples @p channels into the file layout and hands the result to the packetizer.
  Result<NoneType, std::string> convertAndQueue(const float *const *channels, int numFrames);
  void discardOutputFile();

  std::unique_ptr<EncoderState> state_;
};

} // namespace audioapi::ogg_opus

#endif // RN_AUDIO_API_BUNDLED_OGG_OPUS
