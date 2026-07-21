#include <audioapi/decoding/backends/MiniAudioDecoder.h>

#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace audioapi::decoding::miniaudio {

namespace {

std::string parseMiniAudioError(ma_result result) {
  const char *description = ma_result_description(result);
  if (description == nullptr) {
    return "Unknown MiniAudio error (" + std::to_string(result) + ")";
  }
  return std::string(description) + " (" + std::to_string(result) + ")";
}

ma_decoder_config makeDecoderConfig(const int outputSampleRate) {
  const ma_uint32 outRate = outputSampleRate > 0 ? static_cast<ma_uint32>(outputSampleRate) : 0;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, outRate);
#if RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
  config.ppCustomBackendVTables = nullptr;
  config.customBackendCount = 0;
#else
  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  static ma_decoding_backend_vtable *customBackends[] = {
      ma_decoding_backend_libvorbis,
      ma_decoding_backend_libopus,
  };
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);
#endif
  return config;
}

} // namespace

MiniAudioDecoder::~MiniAudioDecoder() {
  close();
}

void MiniAudioDecoder::teardownDecoder() {
  if (decoder_ != nullptr) {
    ma_decoder_uninit(decoder_.get());
    decoder_.reset();
  }
  memoryCopy_.clear();
  outputChannels_ = 0;
  outputSampleRate_ = 0;
  totalOutputFrames_ = 0;
  totalLengthFrames_ = 0;
}

void MiniAudioDecoder::close() {
  teardownDecoder();
}

bool MiniAudioDecoder::isOpen() const {
  return decoder_ != nullptr;
}

int MiniAudioDecoder::outputChannels() const {
  return outputChannels_;
}

int MiniAudioDecoder::outputSampleRate() const {
  return outputSampleRate_;
}

float MiniAudioDecoder::getDurationInSeconds() const {
  if (!isOpen() || outputSampleRate_ <= 0 || totalLengthFrames_ == 0) {
    return 0;
  }
  return static_cast<float>(
      static_cast<double>(totalLengthFrames_) / static_cast<double>(outputSampleRate_));
}

float MiniAudioDecoder::getCurrentPositionInSeconds() const {
  if (!isOpen() || outputSampleRate_ <= 0) {
    return 0;
  }
  return static_cast<float>(
      static_cast<double>(totalOutputFrames_) / static_cast<double>(outputSampleRate_));
}

DecoderResult MiniAudioDecoder::open(const LocalFileSource &source) {
  close();
  if (source.path.empty()) {
    return Err("MiniAudioDecoder::open failed: path is empty");
  }

  ma_decoder_config config = makeDecoderConfig(source.sampleRate);
  decoder_ = std::make_unique<ma_decoder>();
  const ma_result result = ma_decoder_init_file(source.path.c_str(), &config, decoder_.get());
  if (result != MA_SUCCESS) {
    teardownDecoder();
    return Err("MiniAudioDecoder::open failed: " + parseMiniAudioError(result));
  }

  outputChannels_ = static_cast<int>(decoder_->outputChannels);
  outputSampleRate_ = static_cast<int>(decoder_->outputSampleRate);

  ma_uint64 length = 0;
  if (ma_decoder_get_length_in_pcm_frames(decoder_.get(), &length) == MA_SUCCESS) {
    totalLengthFrames_ = static_cast<std::uint64_t>(length);
  } else {
    totalLengthFrames_ = 0;
  }
  totalOutputFrames_ = 0;
  return Ok(None);
}

DecoderResult MiniAudioDecoder::open(const EncodedMemorySource &source) {
  close();
  if (source.data.empty()) {
    return Err("MiniAudioDecoder::open failed: input data is empty");
  }
  memoryCopy_ = source.data;

  ma_decoder_config config = makeDecoderConfig(source.sampleRate);
  decoder_ = std::make_unique<ma_decoder>();
  const ma_result result =
      ma_decoder_init_memory(memoryCopy_.data(), memoryCopy_.size(), &config, decoder_.get());
  if (result != MA_SUCCESS) {
    teardownDecoder();
    return Err("MiniAudioDecoder::open failed: " + parseMiniAudioError(result));
  }

  outputChannels_ = static_cast<int>(decoder_->outputChannels);
  outputSampleRate_ = static_cast<int>(decoder_->outputSampleRate);

  ma_uint64 length = 0;
  if (ma_decoder_get_length_in_pcm_frames(decoder_.get(), &length) == MA_SUCCESS) {
    totalLengthFrames_ = static_cast<std::uint64_t>(length);
  } else {
    totalLengthFrames_ = 0;
  }
  totalOutputFrames_ = 0;
  return Ok(None);
}

size_t MiniAudioDecoder::getTotalPcmFrameCount() const {
  return static_cast<size_t>(totalLengthFrames_);
}

size_t MiniAudioDecoder::readPcmFrames(float *outInterleaved, size_t frameCount) {
  if (!isOpen() || outInterleaved == nullptr || frameCount == 0 || outputChannels_ <= 0) {
    return 0;
  }
  ma_uint64 framesRead = 0;
  ma_decoder_read_pcm_frames(
      decoder_.get(), outInterleaved, static_cast<ma_uint64>(frameCount), &framesRead);
  totalOutputFrames_ += static_cast<size_t>(framesRead);
  return static_cast<size_t>(framesRead);
}

DecoderResult MiniAudioDecoder::seekToTime(double seconds) {
  if (!isOpen() || outputSampleRate_ <= 0) {
    return Err("MiniAudioDecoder::seekToTime failed: decoder is not open");
  }
  const float dur = getDurationInSeconds();
  if (dur > 0 && std::isfinite(dur)) {
    seconds = std::clamp(seconds, 0.0, static_cast<double>(dur));
  } else {
    seconds = std::max(0.0, seconds);
    if (!std::isfinite(seconds)) {
      return Err("MiniAudioDecoder::seekToTime failed: seconds is not finite");
    }
  }

  const auto frame =
      static_cast<ma_uint64>(std::llround(seconds * static_cast<double>(outputSampleRate_)));
  const ma_result result = ma_decoder_seek_to_pcm_frame(decoder_.get(), frame);
  if (result != MA_SUCCESS) {
    return Err("MiniAudioDecoder::seekToTime failed: " + parseMiniAudioError(result));
  }
  totalOutputFrames_ = static_cast<size_t>(frame);
  return Ok(None);
}

} // namespace audioapi::decoding::miniaudio
