#include <audioapi/encoding/AdtsHeader.h>

#include <array>
#include <string>

namespace audioapi::adts {

namespace {

/// Indexed by the sampling frequency index of the AudioSpecificConfig.
constexpr std::array<int, 13> SAMPLING_FREQUENCIES =
    {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350};

constexpr uint8_t SYNCWORD_HIGH = 0xFF;
/// The low 4 sync bits, then ID 0 (MPEG-4), layer 0 and protection_absent 1 (no CRC).
constexpr uint8_t SYNCWORD_LOW_FLAGS = 0xF1;
/// AAC-LC is object type 2, stored as (type - 1) in the 2-bit profile field.
constexpr unsigned PROFILE_AAC_LC = 1;
/// 0x7FF: variable bit rate, no buffer fullness information.
constexpr unsigned BUFFER_FULLNESS_VBR = 0x7FF;

} // namespace

std::optional<uint8_t> samplingFrequencyIndex(int sampleRate) {
  for (size_t index = 0; index < SAMPLING_FREQUENCIES.size(); ++index) {
    if (SAMPLING_FREQUENCIES[index] == sampleRate) {
      return static_cast<uint8_t>(index);
    }
  }
  return std::nullopt;
}

Result<NoneType, std::string> validateLayout(const AudioLayout &layout) {
  const int sampleRate = static_cast<int>(layout.sampleRate);
  if (!samplingFrequencyIndex(sampleRate).has_value()) {
    return Err("Sample rate not expressible in an ADTS header: " + std::to_string(sampleRate));
  }
  if (layout.channelCount < 1 || layout.channelCount > MAX_CHANNEL_CONFIGURATION) {
    return Err(
        "Channel count not expressible in an ADTS header: " + std::to_string(layout.channelCount));
  }
  return Ok(None);
}

std::optional<HeaderBytes>
makeHeader(uint8_t samplingFrequencyIndex, int channelCount, size_t payloadSize) {
  const size_t frameLength = payloadSize + HEADER_SIZE;
  if (frameLength > MAX_FRAME_LENGTH) {
    return std::nullopt;
  }

  const auto channelConfiguration = static_cast<unsigned>(channelCount);
  const auto length = static_cast<unsigned>(frameLength);

  HeaderBytes header{};
  header[0] = SYNCWORD_HIGH;
  header[1] = SYNCWORD_LOW_FLAGS;
  header[2] = static_cast<uint8_t>(
      (PROFILE_AAC_LC << 6) | (static_cast<unsigned>(samplingFrequencyIndex) << 2) |
      ((channelConfiguration >> 2) & 0x1U));
  header[3] = static_cast<uint8_t>(((channelConfiguration & 0x3U) << 6) | ((length >> 11) & 0x3U));
  header[4] = static_cast<uint8_t>((length >> 3) & 0xFFU);
  header[5] = static_cast<uint8_t>(((length & 0x7U) << 5) | ((BUFFER_FULLNESS_VBR >> 6) & 0x1FU));
  // Low 6 bits of buffer fullness, then "1 raw data block per frame" (count - 1 = 0).
  header[6] = static_cast<uint8_t>((BUFFER_FULLNESS_VBR & 0x3FU) << 2);
  return header;
}

std::optional<FrameInfo> parseHeader(const HeaderBytes &header) {
  const bool hasSyncword = header[0] == SYNCWORD_HIGH && (header[1] & 0xF6U) == 0xF0U;
  if (!hasSyncword) {
    return std::nullopt;
  }

  FrameInfo info{
      .frameLength = ((static_cast<size_t>(header[3]) & 0x3U) << 11) |
          (static_cast<size_t>(header[4]) << 3) | (static_cast<size_t>(header[5]) >> 5),
      .samplingFrequencyIndex = static_cast<uint8_t>((header[2] >> 2) & 0xFU),
      .channelConfiguration = static_cast<uint8_t>(((header[2] & 0x1U) << 2) | (header[3] >> 6)),
      .hasCrc = (header[1] & 0x1U) == 0,
  };
  const size_t minimumLength = HEADER_SIZE + (info.hasCrc ? 2 : 0);
  if (info.frameLength < minimumLength) {
    return std::nullopt;
  }
  return info;
}

} // namespace audioapi::adts
