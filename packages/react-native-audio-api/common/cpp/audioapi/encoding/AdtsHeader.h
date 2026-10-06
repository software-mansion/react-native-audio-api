#pragma once

#include <audioapi/utils/AudioLayout.h>
#include <audioapi/utils/Result.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

/// ADTS framing for raw AAC-LC streams (ISO/IEC 13818-7 / 14496-3): every access unit is
/// prefixed with a 7-byte header carrying the sync word, the stream layout and the frame
/// length, so a file cut off at any point stays decodable up to its last complete frame.
namespace audioapi::adts {

inline constexpr size_t HEADER_SIZE = 7;
/// Largest header + payload the 13-bit frame-length field can describe.
inline constexpr size_t MAX_FRAME_LENGTH = (1U << 13) - 1;
/// The 3-bit channel configuration maps 1..6 to the channel count; 7 means 7.1 and 0 means
/// "declared by an in-band PCE", which these headers never carry.
inline constexpr int MAX_CHANNEL_CONFIGURATION = 6;

using HeaderBytes = std::array<uint8_t, HEADER_SIZE>;

/// MPEG-4 sampling frequency index of @p sampleRate, or nullopt when ADTS cannot express it.
std::optional<uint8_t> samplingFrequencyIndex(int sampleRate);

/// Whether @p layout fits in a header; the error names the field that does not.
Result<NoneType, std::string> validateLayout(const AudioLayout &layout);

/// Header for one AAC-LC access unit of @p payloadSize bytes, without CRC. @p samplingFrequencyIndex
/// and @p channelCount come from a layout validateLayout() accepted. Nullopt when the frame
/// would exceed MAX_FRAME_LENGTH.
std::optional<HeaderBytes>
makeHeader(uint8_t samplingFrequencyIndex, int channelCount, size_t payloadSize);

/// What a fixed header declares about its frame.
struct FrameInfo {
  /// Header (and CRC, when present) included.
  size_t frameLength = 0;
  uint8_t samplingFrequencyIndex = 0;
  uint8_t channelConfiguration = 0;
  bool hasCrc = false;

  bool operator==(const FrameInfo &) const = default;
};

/// Reads @p header back; nullopt when the sync word is missing or the frame length could
/// not hold the header. The inverse of makeHeader(), and the way a stream is walked.
std::optional<FrameInfo> parseHeader(const HeaderBytes &header);

} // namespace audioapi::adts
