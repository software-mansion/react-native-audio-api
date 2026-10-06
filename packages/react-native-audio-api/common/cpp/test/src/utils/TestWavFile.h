#pragma once

#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// NOLINTBEGIN

namespace audioapi::test {

inline void writeUint16LE(std::ofstream &output, uint16_t value) {
  output.put(static_cast<char>(value & 0xFF));
  output.put(static_cast<char>((value >> 8) & 0xFF));
}

inline void writeUint32LE(std::ofstream &output, uint32_t value) {
  output.put(static_cast<char>(value & 0xFF));
  output.put(static_cast<char>((value >> 8) & 0xFF));
  output.put(static_cast<char>((value >> 16) & 0xFF));
  output.put(static_cast<char>((value >> 24) & 0xFF));
}

/// Writes interleaved float32 @p frames as an IEEE-float WAV (format tag 3).
inline void writeFloatWavFile(
    const std::string &path,
    const std::vector<float> &frames,
    uint32_t sampleRate,
    uint16_t channelCount) {
  constexpr uint16_t FLOAT_FORMAT_TAG = 3;
  constexpr uint16_t BYTES_PER_SAMPLE = sizeof(float);
  const auto dataBytes = static_cast<uint32_t>(frames.size() * BYTES_PER_SAMPLE);

  std::ofstream output(path, std::ios::binary);
  ASSERT_TRUE(output.is_open());
  output.write("RIFF", 4);
  writeUint32LE(output, 36 + dataBytes);
  output.write("WAVE", 4);
  output.write("fmt ", 4);
  writeUint32LE(output, 16);
  writeUint16LE(output, FLOAT_FORMAT_TAG);
  writeUint16LE(output, channelCount);
  writeUint32LE(output, sampleRate);
  writeUint32LE(output, sampleRate * channelCount * BYTES_PER_SAMPLE);
  writeUint16LE(output, channelCount * BYTES_PER_SAMPLE);
  writeUint16LE(output, BYTES_PER_SAMPLE * 8);
  output.write("data", 4);
  writeUint32LE(output, dataBytes);
  output.write(reinterpret_cast<const char *>(frames.data()), dataBytes);
}

} // namespace audioapi::test

// NOLINTEND
