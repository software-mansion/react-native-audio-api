#include <audioapi/android/AndroidEncoder.h>

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>
#include <media/NdkMediaMuxer.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

// AMEDIAMUXER_OUTPUT_FORMAT_OGG was added to the NDK at API level 34; fall back
// to a numeric value so builds against older NDK headers still compile. Runtime
// availability is still gated by the device OS version.
#ifndef AMEDIAMUXER_OUTPUT_FORMAT_OGG
#define AMEDIAMUXER_OUTPUT_FORMAT_OGG 4
#endif

namespace audioapi::android_encoder {
namespace {

constexpr int64_t kStreamTimeoutUs = 0;
constexpr int64_t kDrainTimeoutUs = 10000;
constexpr int kMaxTryAgainAtEos = 200;
constexpr int kAacProfileLc = 2;     // MediaCodecInfo.CodecProfileLevel.AACObjectLC
constexpr int kPcmEncoding16Bit = 2; // AudioFormat.ENCODING_PCM_16BIT
constexpr int kResampleMaxInFrames = 4096;

int16_t floatToS16(float sample) {
  float clamped = std::max(-1.0f, std::min(1.0f, sample));
  return static_cast<int16_t>(clamped * 32767.0f);
}

struct MediaCodecDeleter {
  void operator()(AMediaCodec *codec) const {
    if (codec != nullptr) {
      AMediaCodec_stop(codec);
      AMediaCodec_delete(codec);
    }
  }
};
using MediaCodecPtr = std::unique_ptr<AMediaCodec, MediaCodecDeleter>;

} // namespace

// ---------------------------------------------------------------------------
// Backend interface
// ---------------------------------------------------------------------------

class IEncoderBackend {
 public:
  virtual ~IEncoderBackend() = default;

  // Opens the backend. `desiredSampleRate`/`desiredChannelCount` come from the
  // file properties; the backend may override them (e.g. Opus) via the out
  // parameters, which the caller then resamples/channel-maps to.
  virtual std::string open(
      int desiredSampleRate,
      int desiredChannelCount,
      const std::string &filePath,
      const std::shared_ptr<AudioFileProperties> &properties,
      const EncoderOutputSpec &outputSpec,
      int &effectiveSampleRate,
      int &effectiveChannelCount) = 0;

  // Encodes frames already in the backend's effective format, one float32 pointer per
  // channel. Backends interleave while they quantize, so no separate interleave pass exists.
  virtual std::string encodePlanar(const float *const *planar, int numFrames) = 0;

  virtual std::string close() = 0;

  [[nodiscard]] virtual size_t getFileSizeBytes() const = 0;
};

// ---------------------------------------------------------------------------
// WAV (RIFF) backend — pure container writing, no third-party codec.
// ---------------------------------------------------------------------------

class WavBackend : public IEncoderBackend {
 public:
  std::string open(
      int desiredSampleRate,
      int desiredChannelCount,
      const std::string &filePath,
      const std::shared_ptr<AudioFileProperties> &properties,
      const EncoderOutputSpec &outputSpec,
      int &effectiveSampleRate,
      int &effectiveChannelCount) override {
    (void)outputSpec;
    sampleRate_ = desiredSampleRate;
    channelCount_ = desiredChannelCount;
    effectiveSampleRate = sampleRate_;
    effectiveChannelCount = channelCount_;

    switch (properties->bitDepth) {
      case AudioFileProperties::BitDepth::Bit16:
        bytesPerSample_ = 2;
        isFloat_ = false;
        break;
      case AudioFileProperties::BitDepth::Bit24:
        bytesPerSample_ = 3;
        isFloat_ = false;
        break;
      case AudioFileProperties::BitDepth::Bit32:
      default:
        bytesPerSample_ = 4;
        isFloat_ = true;
        break;
    }

    file_ = std::fopen(filePath.c_str(), "wb");
    if (file_ == nullptr) {
      return "WavBackend: failed to open file for writing";
    }
    // Reserve the 44-byte canonical WAV header; patched on close.
    std::vector<uint8_t> placeholder(44, 0);
    std::fwrite(placeholder.data(), 1, placeholder.size(), file_);
    dataBytes_ = 0;
    return "";
  }

  std::string encodePlanar(const float *const *planar, int numFrames) override {
    return writeSamples(
        numFrames, [planar](size_t frame, size_t channel) { return planar[channel][frame]; });
  }

  std::string close() override {
    if (file_ == nullptr) {
      return "WavBackend: file not open";
    }
    writeHeader();
    std::fclose(file_);
    file_ = nullptr;
    return "";
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return 44 + dataBytes_;
  }

 private:
  // Walks frames in file order and converts each sample straight into the scratch bytes, so
  // interleaving costs nothing extra whichever layout @p sampleAt reads from.
  template <typename SampleAt>
  std::string writeSamples(int numFrames, SampleAt sampleAt) {
    if (file_ == nullptr) {
      return "WavBackend: file not open";
    }
    const auto frames = static_cast<size_t>(numFrames);
    const auto channels = static_cast<size_t>(channelCount_);
    scratch_.resize(frames * channels * bytesPerSample_);
    uint8_t *out = scratch_.data();

    for (size_t frame = 0; frame < frames; ++frame) {
      for (size_t channel = 0; channel < channels; ++channel) {
        out = writeSample(out, sampleAt(frame, channel));
      }
    }

    size_t written = std::fwrite(scratch_.data(), 1, scratch_.size(), file_);
    if (written != scratch_.size()) {
      return "WavBackend: short write";
    }
    dataBytes_ += scratch_.size();
    return "";
  }

  [[nodiscard]] uint8_t *writeSample(uint8_t *out, float sample) const {
    if (isFloat_) {
      std::memcpy(out, &sample, sizeof(float));
      return out + 4;
    }
    if (bytesPerSample_ == 2) {
      int16_t v = floatToS16(sample);
      std::memcpy(out, &v, sizeof(int16_t));
      return out + 2;
    }
    // 24-bit little-endian PCM
    float clamped = std::max(-1.0f, std::min(1.0f, sample));
    auto v = static_cast<int32_t>(clamped * 8388607.0f);
    out[0] = static_cast<uint8_t>(v & 0xFF);
    out[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    out[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    return out + 3;
  }

  void writeHeader() {
    const uint16_t audioFormatTag = isFloat_ ? 3 : 1; // 3 = IEEE float, 1 = PCM
    const uint16_t bitsPerSample = static_cast<uint16_t>(bytesPerSample_ * 8);
    const uint32_t byteRate = static_cast<uint32_t>(sampleRate_) * channelCount_ * bytesPerSample_;
    const uint16_t blockAlign = static_cast<uint16_t>(channelCount_ * bytesPerSample_);
    const uint32_t dataChunkSize = static_cast<uint32_t>(dataBytes_);
    const uint32_t riffChunkSize = 36 + dataChunkSize;

    uint8_t header[44];
    std::memcpy(header, "RIFF", 4);
    writeU32(header + 4, riffChunkSize);
    std::memcpy(header + 8, "WAVE", 4);
    std::memcpy(header + 12, "fmt ", 4);
    writeU32(header + 16, 16); // PCM fmt chunk size
    writeU16(header + 20, audioFormatTag);
    writeU16(header + 22, static_cast<uint16_t>(channelCount_));
    writeU32(header + 24, static_cast<uint32_t>(sampleRate_));
    writeU32(header + 28, byteRate);
    writeU16(header + 32, blockAlign);
    writeU16(header + 34, bitsPerSample);
    std::memcpy(header + 36, "data", 4);
    writeU32(header + 40, dataChunkSize);

    std::fseek(file_, 0, SEEK_SET);
    std::fwrite(header, 1, sizeof(header), file_);
  }

  static void writeU32(uint8_t *p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
  }
  static void writeU16(uint8_t *p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFF);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
  }

  std::FILE *file_{nullptr};
  int sampleRate_{0};
  int channelCount_{0};
  int bytesPerSample_{2};
  bool isFloat_{false};
  size_t dataBytes_{0};
  std::vector<uint8_t> scratch_;
};

// ---------------------------------------------------------------------------
// MediaCodec base — shared PCM → codec pumping.
// ---------------------------------------------------------------------------

class MediaCodecBackend : public IEncoderBackend {
 public:
  std::string encodePlanar(const float *const *planar, int numFrames) override {
    return queueAndPump(
        numFrames, [planar](size_t frame, size_t channel) { return planar[channel][frame]; });
  }

  std::string close() override {
    if (codec_ == nullptr) {
      return "";
    }

    // Keep feeding (to queue any remaining PCM and the end-of-stream marker) and
    // draining until the codec reports EOS on the output, or we make no progress
    // for too long (defensive against a stuck codec).
    int stagnation = 0;
    while (!outputDone_ && stagnation < kMaxTryAgainAtEos) {
      const size_t drainedBefore = drainCount_;
      const bool inputWasEnded = inputEnded_;

      std::string err = feedInput(true);
      if (!err.empty()) {
        return err;
      }
      err = drainOutput(true);
      if (!err.empty()) {
        return err;
      }

      const bool madeProgress = drainCount_ != drainedBefore || (inputEnded_ && !inputWasEnded);
      stagnation = madeProgress ? 0 : stagnation + 1;
    }
    return finalize();
  }

 protected:
  // Subclass hooks.
  virtual std::string onFormatChanged(AMediaFormat *outputFormat) = 0;
  virtual std::string onEncodedSample(const uint8_t *data, const AMediaCodecBufferInfo &info) = 0;
  virtual std::string finalize() = 0;

  // Creates + starts the encoder for `mime` at the given format.
  std::string
  startCodec(const char *mime, int sampleRate, int channelCount, int bitRate, bool isAac) {
    sampleRate_ = sampleRate;
    channelCount_ = channelCount;

    AMediaFormat *format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, mime);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, sampleRate);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, channelCount);
    if (bitRate > 0) {
      AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, bitRate);
    }
    AMediaFormat_setInt32(format, "pcm-encoding", kPcmEncoding16Bit);
    if (isAac) {
      AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_AAC_PROFILE, kAacProfileLc);
    }

    AMediaCodec *codec = AMediaCodec_createEncoderByType(mime);
    if (codec == nullptr) {
      AMediaFormat_delete(format);
      return std::string("No system encoder available for ") + mime;
    }
    media_status_t status =
        AMediaCodec_configure(codec, format, nullptr, nullptr, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    AMediaFormat_delete(format);
    if (status != AMEDIA_OK) {
      AMediaCodec_delete(codec);
      return std::string("Failed to configure encoder for ") + mime;
    }
    if (AMediaCodec_start(codec) != AMEDIA_OK) {
      AMediaCodec_delete(codec);
      return std::string("Failed to start encoder for ") + mime;
    }
    codec_.reset(codec);
    return "";
  }

  AMediaCodec *codec() {
    return codec_.get();
  }

 private:
  // Quantizes to interleaved 16-bit PCM in frame order, so a planar source is interleaved in
  // the same pass, then pumps the codec.
  template <typename SampleAt>
  std::string queueAndPump(int numFrames, SampleAt sampleAt) {
    if (codec_ == nullptr) {
      return "MediaCodecBackend: codec not started";
    }
    const auto frames = static_cast<size_t>(numFrames);
    const auto channels = static_cast<size_t>(channelCount_);
    const size_t byteOffset = pcmLeftover_.size();
    pcmLeftover_.resize(byteOffset + frames * channels * sizeof(int16_t));
    auto *dst = reinterpret_cast<int16_t *>(pcmLeftover_.data() + byteOffset);
    for (size_t frame = 0; frame < frames; ++frame) {
      for (size_t channel = 0; channel < channels; ++channel) {
        *dst++ = floatToS16(sampleAt(frame, channel));
      }
    }

    std::string err = feedInput(false);
    if (!err.empty()) {
      return err;
    }
    return drainOutput(false);
  }

  std::string feedInput(bool endOfStream) {
    auto *codec = codec_.get();
    size_t cursor = 0;
    while (cursor < pcmLeftover_.size() || (endOfStream && !inputEnded_)) {
      const ssize_t inIndex = AMediaCodec_dequeueInputBuffer(codec, kStreamTimeoutUs);
      if (inIndex < 0) {
        break; // no input buffer free right now
      }
      size_t bufSize = 0;
      uint8_t *buf = AMediaCodec_getInputBuffer(codec, static_cast<size_t>(inIndex), &bufSize);
      if (buf == nullptr) {
        return "MediaCodecBackend: null input buffer";
      }

      const size_t remaining = pcmLeftover_.size() - cursor;
      const size_t toCopy = std::min(remaining, bufSize);
      if (toCopy > 0) {
        std::memcpy(buf, pcmLeftover_.data() + cursor, toCopy);
      }

      const int64_t ptsUs = static_cast<int64_t>(
          static_cast<double>(framesFed_) * 1e6 / static_cast<double>(sampleRate_));

      uint32_t flags = 0;
      const bool lastChunk = endOfStream && (cursor + toCopy >= pcmLeftover_.size());
      if (lastChunk) {
        flags = AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM;
        inputEnded_ = true;
      }

      AMediaCodec_queueInputBuffer(codec, static_cast<size_t>(inIndex), 0, toCopy, ptsUs, flags);

      if (channelCount_ > 0) {
        framesFed_ += toCopy / (sizeof(int16_t) * static_cast<size_t>(channelCount_));
      }
      cursor += toCopy;

      if (lastChunk) {
        break;
      }
    }

    if (cursor >= pcmLeftover_.size()) {
      pcmLeftover_.clear();
    } else if (cursor > 0) {
      pcmLeftover_.erase(
          pcmLeftover_.begin(), pcmLeftover_.begin() + static_cast<std::ptrdiff_t>(cursor));
    }
    return "";
  }

  std::string drainOutput(bool endOfStream) {
    auto *codec = codec_.get();
    const int64_t timeout = endOfStream ? kDrainTimeoutUs : kStreamTimeoutUs;

    while (true) {
      AMediaCodecBufferInfo info{};
      const ssize_t outIndex = AMediaCodec_dequeueOutputBuffer(codec, &info, timeout);
      if (outIndex >= 0) {
        ++drainCount_;
        size_t outSize = 0;
        uint8_t *outBuf =
            AMediaCodec_getOutputBuffer(codec, static_cast<size_t>(outIndex), &outSize);
        if (outBuf != nullptr && info.size > 0) {
          std::string err = onEncodedSample(outBuf, info);
          if (!err.empty()) {
            AMediaCodec_releaseOutputBuffer(codec, static_cast<size_t>(outIndex), false);
            return err;
          }
        }
        AMediaCodec_releaseOutputBuffer(codec, static_cast<size_t>(outIndex), false);
        if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
          outputDone_ = true;
          break;
        }
      } else if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
        AMediaFormat *outFormat = AMediaCodec_getOutputFormat(codec);
        std::string err = onFormatChanged(outFormat);
        AMediaFormat_delete(outFormat);
        if (!err.empty()) {
          return err;
        }
      } else {
        // AMEDIACODEC_INFO_TRY_AGAIN_LATER / OUTPUT_BUFFERS_CHANGED
        break;
      }
    }
    return "";
  }

 protected:
  MediaCodecPtr codec_;
  int sampleRate_{0};
  int channelCount_{0};

 private:
  std::vector<uint8_t> pcmLeftover_;
  uint64_t framesFed_{0};
  bool inputEnded_{false};
  bool outputDone_{false};
  size_t drainCount_{0};
};

// ---------------------------------------------------------------------------
// Muxed backend — AAC / Opus / Vorbis via AMediaMuxer.
// ---------------------------------------------------------------------------

class MuxedBackend : public MediaCodecBackend {
 public:
  std::string open(
      int desiredSampleRate,
      int desiredChannelCount,
      const std::string &filePath,
      const std::shared_ptr<AudioFileProperties> &properties,
      const EncoderOutputSpec &outputSpec,
      int &effectiveSampleRate,
      int &effectiveChannelCount) override {
    int sampleRate = desiredSampleRate;
    int channels = desiredChannelCount;
    const char *mime = "audio/mp4a-latm";
    bool isAac = false;
    int bitRate = static_cast<int>(properties->bitRate);

    switch (outputSpec.codec) {
      case AudioCodec::AAC:
        mime = "audio/mp4a-latm";
        isAac = true;
        muxerFormat_ = AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4;
        break;
      case AudioCodec::OPUS:
        mime = "audio/opus";
        sampleRate = 48000;
        if (bitRate <= 0) {
          bitRate = 96000;
        }
        muxerFormat_ = outputSpec.container == AudioContainer::OGG ? AMEDIAMUXER_OUTPUT_FORMAT_OGG
                                                                   : AMEDIAMUXER_OUTPUT_FORMAT_WEBM;
        break;
      case AudioCodec::VORBIS:
        mime = "audio/vorbis";
        if (bitRate <= 0) {
          bitRate = 128000;
        }
        muxerFormat_ = AMEDIAMUXER_OUTPUT_FORMAT_WEBM;
        break;
      default:
        return "MuxedBackend: unsupported codec";
    }

    effectiveSampleRate = sampleRate;
    effectiveChannelCount = channels;

    fd_ = ::open(filePath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd_ < 0) {
      return "MuxedBackend: failed to open file descriptor";
    }
    muxer_ = AMediaMuxer_new(fd_, static_cast<OutputFormat>(muxerFormat_));
    if (muxer_ == nullptr) {
      ::close(fd_);
      fd_ = -1;
      return "MuxedBackend: failed to create muxer";
    }

    std::string err = startCodec(mime, sampleRate, channels, bitRate, isAac);
    if (!err.empty()) {
      AMediaMuxer_delete(muxer_);
      muxer_ = nullptr;
      ::close(fd_);
      fd_ = -1;
      return err;
    }
    filePath_ = filePath;
    return "";
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    struct stat st{};
    if (!filePath_.empty() && ::stat(filePath_.c_str(), &st) == 0) {
      return static_cast<size_t>(st.st_size);
    }
    return 0;
  }

 protected:
  std::string onFormatChanged(AMediaFormat *outputFormat) override {
    if (muxerStarted_) {
      return "";
    }
    trackIndex_ = AMediaMuxer_addTrack(muxer_, outputFormat);
    if (trackIndex_ < 0) {
      return "MuxedBackend: addTrack failed";
    }
    if (AMediaMuxer_start(muxer_) != AMEDIA_OK) {
      return "MuxedBackend: muxer start failed";
    }
    muxerStarted_ = true;
    return "";
  }

  std::string onEncodedSample(const uint8_t *data, const AMediaCodecBufferInfo &info) override {
    // Codec-specific data is carried in the output format for the muxer; skip it.
    if ((info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) != 0) {
      return "";
    }
    if (!muxerStarted_) {
      // Some encoders emit the first frame before a format-changed event; the
      // muxer must be started first.
      AMediaFormat *outFormat = AMediaCodec_getOutputFormat(codec());
      std::string err = onFormatChanged(outFormat);
      AMediaFormat_delete(outFormat);
      if (!err.empty()) {
        return err;
      }
    }
    AMediaCodecBufferInfo writeInfo = info;
    if (AMediaMuxer_writeSampleData(muxer_, static_cast<size_t>(trackIndex_), data, &writeInfo) !=
        AMEDIA_OK) {
      return "MuxedBackend: writeSampleData failed";
    }
    return "";
  }

  std::string finalize() override {
    codec_.reset();
    if (muxer_ != nullptr) {
      if (muxerStarted_) {
        AMediaMuxer_stop(muxer_);
      }
      AMediaMuxer_delete(muxer_);
      muxer_ = nullptr;
    }
    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
    return "";
  }

 private:
  AMediaMuxer *muxer_{nullptr};
  int fd_{-1};
  int muxerFormat_{AMEDIAMUXER_OUTPUT_FORMAT_MPEG_4};
  ssize_t trackIndex_{-1};
  bool muxerStarted_{false};
  std::string filePath_;
};

// ---------------------------------------------------------------------------
// FLAC backend — MediaCodec FLAC encoder, raw stream written to file.
// ---------------------------------------------------------------------------

class FlacBackend : public MediaCodecBackend {
 public:
  std::string open(
      int desiredSampleRate,
      int desiredChannelCount,
      const std::string &filePath,
      const std::shared_ptr<AudioFileProperties> &properties,
      const EncoderOutputSpec &outputSpec,
      int &effectiveSampleRate,
      int &effectiveChannelCount) override {
    (void)outputSpec;
    effectiveSampleRate = desiredSampleRate;
    effectiveChannelCount = desiredChannelCount;

    file_ = std::fopen(filePath.c_str(), "wb");
    if (file_ == nullptr) {
      return "FlacBackend: failed to open file for writing";
    }
    filePath_ = filePath;

    // FLAC has no MediaMuxer container; the codec emits "fLaC" + STREAMINFO as
    // codec-config, followed by raw FLAC frames — concatenating yields a valid
    // .flac file. compression-level is advisory.
    int bitRate = 0; // lossless; ignored
    std::string err =
        startCodec("audio/flac", desiredSampleRate, desiredChannelCount, bitRate, false);
    if (!err.empty()) {
      std::fclose(file_);
      file_ = nullptr;
      return err;
    }
    return "";
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return bytesWritten_;
  }

 protected:
  std::string onFormatChanged(AMediaFormat *outputFormat) override {
    (void)outputFormat;
    return "";
  }

  std::string onEncodedSample(const uint8_t *data, const AMediaCodecBufferInfo &info) override {
    if (file_ == nullptr) {
      return "FlacBackend: file not open";
    }
    size_t written = std::fwrite(data + info.offset, 1, static_cast<size_t>(info.size), file_);
    if (written != static_cast<size_t>(info.size)) {
      return "FlacBackend: short write";
    }
    bytesWritten_ += written;
    return "";
  }

  std::string finalize() override {
    codec_.reset();
    if (file_ != nullptr) {
      std::fclose(file_);
      file_ = nullptr;
    }
    return "";
  }

 private:
  std::FILE *file_{nullptr};
  std::string filePath_;
  size_t bytesWritten_{0};
};

// ---------------------------------------------------------------------------
// Conversion state — planar scratch for channel mapping and rate conversion, built at
// open() only when the input differs from the backend's effective format.
// ---------------------------------------------------------------------------

struct AndroidEncoder::ConversionState {
  ConversionState(
      int srcRate,
      int dstRate,
      int inputChannels,
      int outputChannels,
      size_t maxInputFrames)
      : maxInputFrames(maxInputFrames) {
    if (inputChannels != outputChannels) {
      inputPlanar =
          std::make_unique<AudioBuffer>(maxInputFrames, inputChannels, static_cast<float>(srcRate));
      mappedPlanar = std::make_unique<AudioBuffer>(
          maxInputFrames, outputChannels, static_cast<float>(srcRate));
    }
    if (srcRate != dstRate) {
      resampler = std::make_unique<r8b::MultiChannelResampler>(
          srcRate, dstRate, outputChannels, kResampleMaxInFrames);
      resampledPlanar = std::make_unique<AudioBuffer>(
          static_cast<size_t>(std::max(1, resampler->getMaxOutLen())),
          outputChannels,
          static_cast<float>(dstRate));
    }
  }

  size_t maxInputFrames;
  // Channel mapping only: the input is staged in inputPlanar and remixed into mappedPlanar.
  std::unique_ptr<AudioBuffer> inputPlanar;
  std::unique_ptr<AudioBuffer> mappedPlanar;
  // Rate conversion only.
  std::unique_ptr<r8b::MultiChannelResampler> resampler;
  std::unique_ptr<AudioBuffer> resampledPlanar;
};

// ---------------------------------------------------------------------------
// AndroidEncoder
// ---------------------------------------------------------------------------

AndroidEncoder::AndroidEncoder(const std::shared_ptr<AudioFileProperties> &fileProperties)
    : AudioEncoder(fileProperties) {}

AndroidEncoder::~AndroidEncoder() {
  if (isOpen()) {
    close();
  }
}

OpenEncoderResult AndroidEncoder::open(
    const StreamFormat &inputFormat,
    const EncoderOutputSpec &outputSpec,
    size_t maxBufferSizeInFrames,
    const std::string &filePath) {
  if (isOpen()) {
    return OpenEncoderResult::Err("Encoder already open");
  }
  if (inputFormat.sampleRate <= 0 || inputFormat.channelCount <= 0) {
    return OpenEncoderResult::Err("Invalid input format");
  }

  inputFormat_ = inputFormat;
  outputSpec_ = outputSpec;
  maxBufferSizeInFrames_ = maxBufferSizeInFrames;
  filePath_ = filePath;
  inputSampleRate_ = inputFormat.sampleRate;
  inputChannelCount_ = inputFormat.channelCount;
  resetFramesEncoded();

  switch (outputSpec.codec) {
    case AudioCodec::PCM:
      backend_ = std::make_unique<WavBackend>();
      break;
    case AudioCodec::FLAC:
      backend_ = std::make_unique<FlacBackend>();
      break;
    case AudioCodec::AAC:
    case AudioCodec::OPUS:
    case AudioCodec::VORBIS:
      backend_ = std::make_unique<MuxedBackend>();
      break;
    default:
      return OpenEncoderResult::Err(
          std::string(toString(outputSpec.codec)) + " is not encodable on Android");
  }

  int effectiveSampleRate = static_cast<int>(fileProperties_->sampleRate);
  int effectiveChannelCount = fileProperties_->channelCount;
  std::string err = backend_->open(
      static_cast<int>(fileProperties_->sampleRate),
      fileProperties_->channelCount,
      filePath,
      fileProperties_,
      outputSpec,
      effectiveSampleRate,
      effectiveChannelCount);
  if (!err.empty()) {
    backend_.reset();
    return OpenEncoderResult::Err(err);
  }

  outputSampleRate_ = effectiveSampleRate;
  outputChannelCount_ = effectiveChannelCount;

  const bool needsConversion = inputChannelCount_ != outputChannelCount_ ||
      static_cast<int>(inputSampleRate_) != outputSampleRate_;
  if (needsConversion) {
    if (inputChannelCount_ > MAX_CHANNEL_COUNT || outputChannelCount_ > MAX_CHANNEL_COUNT) {
      backend_->close();
      backend_.reset();
      return OpenEncoderResult::Err("Channel count exceeds MAX_CHANNEL_COUNT");
    }
    conversion_ = std::make_unique<ConversionState>(
        static_cast<int>(inputSampleRate_),
        outputSampleRate_,
        inputChannelCount_,
        outputChannelCount_,
        std::max<size_t>(maxBufferSizeInFrames, 1));
  }

  markOpen();
  return OpenEncoderResult::Ok(filePath_);
}

std::string AndroidEncoder::encodeConverted(const float *const *channels, int numFrames) {
  ConversionState &state = *conversion_;
  const auto frames = static_cast<size_t>(numFrames);
  if (frames > state.maxInputFrames) {
    return "Encode input exceeds the buffer size declared at open()";
  }

  std::array<const float *, MAX_CHANNEL_COUNT> planar{};
  if (state.mappedPlanar != nullptr) {
    for (int channel = 0; channel < inputChannelCount_; ++channel) {
      std::memcpy(
          state.inputPlanar->getChannel(channel)->begin(),
          channels[channel],
          frames * sizeof(float));
    }
    state.mappedPlanar->copy(*state.inputPlanar, 0, 0, frames);
    for (int channel = 0; channel < outputChannelCount_; ++channel) {
      planar[channel] = state.mappedPlanar->getChannel(channel)->begin();
    }
  } else {
    for (int channel = 0; channel < outputChannelCount_; ++channel) {
      planar[channel] = channels[channel];
    }
  }

  if (state.resampler == nullptr) {
    return backend_->encodePlanar(planar.data(), numFrames);
  }

  std::array<float *, MAX_CHANNEL_COUNT> resampled{};
  for (int channel = 0; channel < outputChannelCount_; ++channel) {
    resampled[channel] = state.resampledPlanar->getChannel(channel)->begin();
  }

  int consumed = 0;
  while (consumed < numFrames) {
    const int chunk = std::min(numFrames - consumed, kResampleMaxInFrames);
    std::array<const float *, MAX_CHANNEL_COUNT> chunkChannels{};
    for (int channel = 0; channel < outputChannelCount_; ++channel) {
      chunkChannels[channel] = planar[channel] + consumed;
    }
    const int produced = state.resampler->process(chunkChannels.data(), chunk, resampled.data());
    consumed += chunk;
    if (produced <= 0) {
      continue;
    }
    std::string err = backend_->encodePlanar(resampled.data(), produced);
    if (!err.empty()) {
      return err;
    }
  }
  return "";
}

EncodeResult AndroidEncoder::encode(const float *const *channels, int numFrames) {
  if (!isOpen() || backend_ == nullptr) {
    return EncodeResult::Err("Encoder is not open");
  }
  if (channels == nullptr || numFrames <= 0) {
    return EncodeResult::Err("Invalid encode input");
  }

  // Frames already in the backend's format go straight through; the backend interleaves
  // while it quantizes, so no layout pass happens here (see encodeConverted).
  std::string err = conversion_ != nullptr ? encodeConverted(channels, numFrames)
                                           : backend_->encodePlanar(channels, numFrames);
  if (!err.empty()) {
    return EncodeResult::Err(err);
  }

  addEncodedFrames(static_cast<size_t>(numFrames));
  return EncodeResult::Ok(static_cast<size_t>(numFrames));
}

CloseEncoderResult AndroidEncoder::close() {
  if (!isOpen() || backend_ == nullptr) {
    return CloseEncoderResult::Err("Encoder is not open");
  }
  markClosed();

  std::string err = backend_->close();
  const size_t sizeBytes = backend_->getFileSizeBytes();
  const double durationSeconds = inputSampleRate_ > 0
      ? static_cast<double>(framesEncoded_.load(std::memory_order_acquire)) / inputSampleRate_
      : 0.0;
  backend_.reset();
  conversion_.reset();
  resetFramesEncoded();

  if (!err.empty()) {
    return CloseEncoderResult::Err(err);
  }

  const double sizeMB = static_cast<double>(sizeBytes) / (1024.0 * 1024.0);
  return CloseEncoderResult::Ok(std::make_tuple(sizeMB, durationSeconds));
}

size_t AndroidEncoder::getFileSizeBytes() const {
  if (backend_ == nullptr) {
    return 0;
  }
  return backend_->getFileSizeBytes();
}

} // namespace audioapi::android_encoder
