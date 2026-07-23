#include <audioapi/decoding/backends/RawPcmDecoder.h>
#include <audioapi/libs/base64/base64.h>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace audioapi::decoding::raw_pcm {
namespace {

float uint8ToFloat(uint8_t byte1, uint8_t byte2) {
  return static_cast<float>(static_cast<int16_t>((byte2 << 8) | byte1)) /
      static_cast<float>(INT16_MAX);
}

} // namespace

DecoderResult RawPcmDecoder::openFromBytes(
    const uint8_t *data,
    size_t size,
    int sampleRate,
    int channelCount,
    bool interleaved) {
  close();
  if (data == nullptr || size == 0) {
    return Err("RawPcmDecoder::open failed: input data is empty");
  }
  if (sampleRate <= 0 || !std::isfinite(sampleRate)) {
    return Err("RawPcmDecoder::open failed: invalid sample rate");
  }
  if (channelCount <= 0) {
    return Err("RawPcmDecoder::open failed: invalid channel count");
  }

  const size_t bytesPerFrame = static_cast<size_t>(channelCount) * sizeof(int16_t);
  if (size % bytesPerFrame != 0) {
    return Err("RawPcmDecoder::open failed: PCM byte length is not frame-aligned");
  }

  const size_t numFrames = size / bytesPerFrame;
  interleavedPcm_.resize(numFrames * static_cast<size_t>(channelCount));

  for (int ch = 0; ch < channelCount; ++ch) {
    for (size_t i = 0; i < numFrames; ++i) {
      size_t offset;
      if (interleaved) {
        offset =
            (i * static_cast<size_t>(channelCount) + static_cast<size_t>(ch)) * sizeof(int16_t);
      } else {
        offset = (static_cast<size_t>(ch) * numFrames + i) * sizeof(int16_t);
      }
      interleavedPcm_[i * static_cast<size_t>(channelCount) + static_cast<size_t>(ch)] =
          uint8ToFloat(data[offset], data[offset + 1]);
    }
  }

  outputChannels_ = channelCount;
  outputSampleRate_ = sampleRate;
  framePosition_ = 0;
  totalPcmFrames_ = numFrames;
  return Ok(None);
}

DecoderResult RawPcmDecoder::open(const RawPcmSource &source) {
  return openFromBytes(
      source.data.data(),
      source.data.size(),
      source.sampleRate,
      source.channelCount,
      source.interleaved);
}

DecoderResult RawPcmDecoder::open(const RawPcmBase64Source &source) {
  auto decodedData = base64_decode(source.base64, false);
  return openFromBytes(
      reinterpret_cast<const uint8_t *>(decodedData.data()),
      decodedData.size(),
      source.sampleRate,
      source.channelCount,
      source.interleaved);
}

size_t RawPcmDecoder::readPcmFrames(float *outInterleaved, size_t frameCount) {
  if (!isOpen() || outInterleaved == nullptr || frameCount == 0 || outputChannels_ <= 0) {
    return 0;
  }

  if (framePosition_ < 0 || static_cast<size_t>(framePosition_) >= totalPcmFrames_) {
    return 0;
  }

  const size_t framesToRead =
      std::min(frameCount, totalPcmFrames_ - static_cast<size_t>(framePosition_));
  const auto channelCount = static_cast<size_t>(outputChannels_);
  const size_t srcOffset = static_cast<size_t>(framePosition_) * channelCount;
  std::copy_n(
      interleavedPcm_.begin() + static_cast<std::ptrdiff_t>(srcOffset),
      framesToRead * channelCount,
      outInterleaved);
  framePosition_ += static_cast<int64_t>(framesToRead);
  return framesToRead;
}

DecoderResult RawPcmDecoder::seekToTime(double seconds) {
  if (!isOpen() || outputSampleRate_ <= 0) {
    return Err("RawPcmDecoder::seekToTime failed: decoder is not open");
  }
  if (!std::isfinite(seconds)) {
    return Err("RawPcmDecoder::seekToTime failed: seconds is not finite");
  }

  const float duration = getDurationInSeconds();
  if (duration > 0.0f) {
    seconds = std::clamp(seconds, 0.0, static_cast<double>(duration));
  } else {
    seconds = std::max(0.0, seconds);
  }

  framePosition_ =
      static_cast<int64_t>(std::llround(seconds * static_cast<double>(outputSampleRate_)));
  framePosition_ = std::min(framePosition_, static_cast<int64_t>(totalPcmFrames_));
  return Ok(None);
}

void RawPcmDecoder::close() {
  interleavedPcm_.clear();
  resetOpenMetadata();
}

bool RawPcmDecoder::isOpen() const {
  return !interleavedPcm_.empty() && outputChannels_ > 0 && outputSampleRate_ > 0;
}

} // namespace audioapi::decoding::raw_pcm
