#include <audioapi/android/AndroidDecoding.h>
#include <audioapi/android/AndroidDecodingDataSource.h>

#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <android/api-level.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>

#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::android_decoder {
namespace {

// Codec output PCM encodings (android.media.AudioFormat constants).
constexpr int kEncodingPcm16 = 2;
constexpr int kEncodingPcm8 = 3;
constexpr int kEncodingPcmFloat = 4;
constexpr int kResampleMaxInFrames = 4096;
constexpr int64_t kCodecTimeoutUs = 5000;
// Bail out of the drain loop if MediaCodec keeps saying "try again" after EOS
// input without ever flagging EOS output (defensive against a stuck codec).
constexpr int kMaxTryAgainAfterEos = 200;

template <auto DeleteFn>
struct FnDeleter {
  void operator()(auto *ptr) const {
    if (ptr != nullptr) {
      DeleteFn(ptr);
    }
  }
};

struct MediaCodecDeleter {
  void operator()(AMediaCodec *codec) const {
    if (codec != nullptr) {
      AMediaCodec_stop(codec);
      AMediaCodec_delete(codec);
    }
  }
};

using MediaExtractorPtr = std::unique_ptr<AMediaExtractor, FnDeleter<AMediaExtractor_delete>>;
using MediaCodecPtr = std::unique_ptr<AMediaCodec, MediaCodecDeleter>;
using FilePtr = std::unique_ptr<FILE, FnDeleter<fclose>>;

decoding::DecoderResult attachMemoryExtractorViaTempFile(
    AMediaExtractor *extractor,
    FilePtr &tempFile,
    const std::vector<uint8_t> &data) {
  tempFile.reset(tmpfile());
  if (tempFile == nullptr || fwrite(data.data(), 1, data.size(), tempFile.get()) != data.size()) {
    return Err("AndroidDecoder::open: temp file write failed");
  }
  fflush(tempFile.get());
  const int fd = fileno(tempFile.get());
  if (fd < 0) {
    return Err("AndroidDecoder::open: fileno failed");
  }

  if (AMediaExtractor_setDataSourceFd(extractor, fd, 0, static_cast<off64_t>(data.size())) !=
      AMEDIA_OK) {
    return Err("AndroidDecoder::open setDataSourceFd failed");
  }
  return Ok(None);
}

} // namespace

// Native reader state, reached through AndroidDecoder::impl_. Kept out of the
// shared header so NDK types never leak into cross-platform code.
struct AndroidDecoderState {
  // Declaration order matters: destroy codec before extractor; keep encoded
  // bytes / temp file / data source alive until the extractor is torn down.
  std::vector<uint8_t> encodedMemory;
  MemoryDataSourceContext memorySourceContext{};
  FilePtr tempFile;
  AndroidMemoryDataSource dataSource;
  MediaExtractorPtr extractor;
  MediaCodecPtr codec;

  int audioTrackIndex = -1;
  std::string codecMime;
  // Requested client rate from open() (0 = follow codec native rate).
  int requestedOutputSampleRate = 0;
  int nativeSampleRate = 0;
  int outputRate = 0;
  int channels = 0;
  int pcmEncoding = kEncodingPcm16;
  double durationSeconds = 0.0;

  // Decoded PCM at the codec's native rate, interleaved float, waiting to be
  // served (no resampling) or fed into the resampler.
  std::vector<float> nativeLeftover;
  size_t nativeCursor = 0;
  // Resampled PCM at the requested output rate, interleaved float.
  std::vector<float> outLeftover;
  size_t outCursor = 0;

  bool inputEnded = false;
  bool outputEnded = false;
  int tryAgainAfterEos = 0;

  // Present only when native rate differs from the requested output rate.
  std::unique_ptr<r8b::MultiChannelResampler> resampler;
  std::unique_ptr<AudioBuffer> resampleIn;
  std::unique_ptr<AudioBuffer> resampleOut;
};

namespace {

void compactConsumed(std::vector<float> &buffer, size_t &cursor) {
  if (cursor == 0) {
    return;
  }
  if (cursor >= buffer.size()) {
    buffer.clear();
  } else {
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(cursor));
  }
  cursor = 0;
}

void appendNativePcm(AndroidDecoderState &state, const uint8_t *bytes, size_t byteCount) {
  compactConsumed(state.nativeLeftover, state.nativeCursor);
  auto &pcm = state.nativeLeftover;
  if (state.pcmEncoding == kEncodingPcmFloat) {
    const size_t sampleCount = byteCount / sizeof(float);
    const auto *samples = reinterpret_cast<const float *>(bytes);
    pcm.insert(pcm.end(), samples, samples + sampleCount);
    return;
  }
  if (state.pcmEncoding == kEncodingPcm8) {
    for (size_t i = 0; i < byteCount; ++i) {
      pcm.push_back((static_cast<float>(bytes[i]) - 128.0f) / 128.0f);
    }
    return;
  }
  const size_t sampleCount = byteCount / sizeof(int16_t);
  const auto *pcm16 = reinterpret_cast<const int16_t *>(bytes);
  for (size_t i = 0; i < sampleCount; ++i) {
    pcm.push_back(static_cast<float>(pcm16[i]) / 32768.0f);
  }
}

void setupResamplerIfNeeded(AndroidDecoderState &state) {
  if (state.channels <= 0) {
    state.channels = 1;
  }
  if (state.nativeSampleRate <= 0) {
    state.nativeSampleRate = 44100;
  }

  state.outputRate = state.requestedOutputSampleRate > 0 ? state.requestedOutputSampleRate
                                                         : state.nativeSampleRate;

  if (state.outputRate == state.nativeSampleRate) {
    state.resampler.reset();
    state.resampleIn.reset();
    state.resampleOut.reset();
    return;
  }

  state.resampler = std::make_unique<r8b::MultiChannelResampler>(
      state.nativeSampleRate, state.outputRate, state.channels, kResampleMaxInFrames);
  state.resampleIn = std::make_unique<AudioBuffer>(
      static_cast<size_t>(kResampleMaxInFrames),
      state.channels,
      static_cast<float>(state.nativeSampleRate));
  state.resampleOut = std::make_unique<AudioBuffer>(
      static_cast<size_t>(std::max(1, state.resampler->getMaxOutLen())),
      state.channels,
      static_cast<float>(state.outputRate));
}

// Applies MediaCodec output format. When rate/channels change vs track metadata
// (common with AAC), rebuilds the resampler and drops PCM produced under the
// previous layout so leftover samples are not misinterpreted.
void applyCodecOutputFormat(AndroidDecoderState &state, AMediaFormat *outFormat) {
  const int previousChannels = state.channels;
  const int previousNativeRate = state.nativeSampleRate;

  AMediaFormat_getInt32(outFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, &state.nativeSampleRate);
  AMediaFormat_getInt32(outFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &state.channels);
  if (!AMediaFormat_getInt32(outFormat, "pcm-encoding", &state.pcmEncoding)) {
    state.pcmEncoding = kEncodingPcm16;
  }
  if (state.channels <= 0) {
    state.channels = previousChannels > 0 ? previousChannels : 1;
  }
  if (state.nativeSampleRate <= 0) {
    state.nativeSampleRate = previousNativeRate > 0 ? previousNativeRate : 44100;
  }

  if (state.channels == previousChannels && state.nativeSampleRate == previousNativeRate) {
    return;
  }

  state.nativeLeftover.clear();
  state.nativeCursor = 0;
  state.outLeftover.clear();
  state.outCursor = 0;
  setupResamplerIfNeeded(state);
}

// Advances the codec one step: feeds at most one input packet and drains at
// most one output buffer into nativeLeftover. Sets outputEnded at EOS.
void pumpCodec(AndroidDecoderState &state) {
  if (state.outputEnded) {
    return;
  }

  auto *codec = state.codec.get();
  auto *extractor = state.extractor.get();

  if (!state.inputEnded) {
    const ssize_t inIndex = AMediaCodec_dequeueInputBuffer(codec, kCodecTimeoutUs);
    if (inIndex >= 0) {
      size_t bufSize = 0;
      uint8_t *buf = AMediaCodec_getInputBuffer(codec, static_cast<size_t>(inIndex), &bufSize);
      const ssize_t sampleSize =
          buf != nullptr ? AMediaExtractor_readSampleData(extractor, buf, bufSize) : -1;
      if (sampleSize < 0) {
        AMediaCodec_queueInputBuffer(
            codec, static_cast<size_t>(inIndex), 0, 0, 0, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
        state.inputEnded = true;
      } else {
        const int64_t pts = AMediaExtractor_getSampleTime(extractor);
        AMediaCodec_queueInputBuffer(
            codec, static_cast<size_t>(inIndex), 0, static_cast<size_t>(sampleSize), pts, 0);
        AMediaExtractor_advance(extractor);
      }
    }
  }

  AMediaCodecBufferInfo info{};
  const ssize_t outIndex = AMediaCodec_dequeueOutputBuffer(codec, &info, kCodecTimeoutUs);
  if (outIndex >= 0) {
    state.tryAgainAfterEos = 0;
    if (info.size > 0) {
      size_t outBufSize = 0;
      uint8_t *outBuf =
          AMediaCodec_getOutputBuffer(codec, static_cast<size_t>(outIndex), &outBufSize);
      if (outBuf != nullptr) {
        appendNativePcm(state, outBuf + info.offset, static_cast<size_t>(info.size));
      }
    }
    AMediaCodec_releaseOutputBuffer(codec, static_cast<size_t>(outIndex), false);
    if ((info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) != 0) {
      state.outputEnded = true;
    }
  } else if (outIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
    AMediaFormat *outFormat = AMediaCodec_getOutputFormat(codec);
    if (outFormat != nullptr) {
      applyCodecOutputFormat(state, outFormat);
      AMediaFormat_delete(outFormat);
    }
  } else if (outIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
    if (state.inputEnded && ++state.tryAgainAfterEos > kMaxTryAgainAfterEos) {
      state.outputEnded = true;
    }
  }
}

size_t availableFrames(const std::vector<float> &buffer, size_t cursor, int channels) {
  if (channels <= 0 || cursor >= buffer.size()) {
    return 0;
  }
  return (buffer.size() - cursor) / static_cast<size_t>(channels);
}

// Pulls native PCM through the resampler, appending output-rate PCM to
// outLeftover. Returns the number of output frames produced; 0 means true EOF
// (native stream exhausted). The resampler has warm-up latency and may emit 0
// frames for a small input block, so we keep feeding until it produces output
// or the codec is drained — returning 0 early would be misread as end-of-stream
// by callers (and could trigger a loop-to-start seek).
size_t produceResampled(AndroidDecoderState &state) {
  while (true) {
    const int ch = state.channels;

    while (availableFrames(state.nativeLeftover, state.nativeCursor, ch) == 0 &&
           !state.outputEnded) {
      pumpCodec(state);
    }

    // Format change during pump may drop the resampler when native rate now
    // matches the requested output rate — hand control back to the native path.
    if (state.resampler == nullptr) {
      if (availableFrames(state.nativeLeftover, state.nativeCursor, state.channels) > 0 ||
          !state.outputEnded) {
        return 1;
      }
      return 0;
    }

    // Format may have changed during pumpCodec; re-read channel count.
    const int channels = state.channels;
    const size_t availIn = availableFrames(state.nativeLeftover, state.nativeCursor, channels);
    if (availIn == 0) {
      return 0; // Codec fully drained and no leftover: genuine EOF.
    }

    const auto inFrames = static_cast<int>(std::min<size_t>(availIn, kResampleMaxInFrames));
    state.resampleIn->deinterleaveFrom(
        state.nativeLeftover.data() + state.nativeCursor, static_cast<size_t>(inFrames));
    state.nativeCursor += static_cast<size_t>(inFrames) * static_cast<size_t>(channels);

    const int outFrames = state.resampler->process(*state.resampleIn, inFrames, *state.resampleOut);
    if (outFrames <= 0) {
      // Resampler still warming up: feed more input rather than signalling EOF.
      continue;
    }

    compactConsumed(state.outLeftover, state.outCursor);
    const size_t base = state.outLeftover.size();
    state.outLeftover.resize(base + static_cast<size_t>(outFrames) * static_cast<size_t>(channels));
    state.resampleOut->interleaveTo(
        state.outLeftover.data() + base, static_cast<size_t>(outFrames));
    return static_cast<size_t>(outFrames);
  }
}

// Selects the audio track and reads container metadata. Metadata channels/rate
// are provisional — settleCodecOutputFormat() replaces them with what MediaCodec
// actually produces before open() publishes them.
decoding::DecoderResult configureExtractorMetadata(
    AndroidDecoderState &state,
    int outputSampleRate) {
  auto *extractor = state.extractor.get();
  const auto trackCount = static_cast<int>(AMediaExtractor_getTrackCount(extractor));
  AMediaFormat *format = nullptr;

  for (int i = 0; i < trackCount; ++i) {
    AMediaFormat *candidate = AMediaExtractor_getTrackFormat(extractor, i);
    const char *mime = nullptr;
    if (AMediaFormat_getString(candidate, AMEDIAFORMAT_KEY_MIME, &mime) && mime != nullptr &&
        std::strncmp(mime, "audio/", 6) == 0) {
      state.audioTrackIndex = i;
      format = candidate;
      break;
    }
    AMediaFormat_delete(candidate);
  }

  if (state.audioTrackIndex < 0 || format == nullptr) {
    return Err("AndroidDecoder: no audio track found");
  }

  if (AMediaExtractor_selectTrack(extractor, state.audioTrackIndex) != AMEDIA_OK) {
    AMediaFormat_delete(format);
    return Err("AndroidDecoder: selectTrack failed");
  }

  const char *mime = nullptr;
  AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);
  state.codecMime = mime != nullptr ? mime : "audio/mp4a-latm";
  AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_SAMPLE_RATE, &state.nativeSampleRate);
  AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &state.channels);
  int64_t durationUs = 0;
  AMediaFormat_getInt64(format, AMEDIAFORMAT_KEY_DURATION, &durationUs);
  AMediaFormat_delete(format);

  state.durationSeconds = durationUs > 0 ? static_cast<double>(durationUs) / 1e6 : 0.0;
  state.requestedOutputSampleRate = outputSampleRate;
  setupResamplerIfNeeded(state);
  return Ok(None);
}

decoding::DecoderResult ensureCodecStarted(AndroidDecoderState &state) {
  if (state.codec != nullptr) {
    return Ok(None);
  }
  if (state.extractor == nullptr || state.audioTrackIndex < 0) {
    return Err("AndroidDecoder: extractor is not configured");
  }

  AMediaFormat *format =
      AMediaExtractor_getTrackFormat(state.extractor.get(), state.audioTrackIndex);
  if (format == nullptr) {
    return Err("AndroidDecoder: getTrackFormat failed");
  }

  AMediaCodec *codec = AMediaCodec_createDecoderByType(state.codecMime.c_str());
  if (codec == nullptr) {
    AMediaFormat_delete(format);
    return Err("AndroidDecoder: createDecoderByType failed");
  }
  if (AMediaCodec_configure(codec, format, nullptr, nullptr, 0) != AMEDIA_OK) {
    AMediaCodec_delete(codec);
    AMediaFormat_delete(format);
    return Err("AndroidDecoder: configure failed");
  }
  AMediaFormat_delete(format);

  if (AMediaCodec_start(codec) != AMEDIA_OK) {
    AMediaCodec_delete(codec);
    return Err("AndroidDecoder: start failed");
  }

  state.codec.reset(codec);
  state.inputEnded = false;
  state.outputEnded = false;
  state.tryAgainAfterEos = 0;
  return Ok(None);
}

// Starts the codec and pumps it until it commits to an output format, so
// channels/rate are final before callers size buffers against outputChannels().
// Track metadata can disagree with what the codec produces — HE-AAC v2 declares
// one channel in the container and decodes to two — and learning that only
// during the first read would overflow buffers already sized from the metadata.
// Frames decoded here stay in nativeLeftover and are served by the first read.
decoding::DecoderResult settleCodecOutputFormat(AndroidDecoderState &state) {
  if (auto started = ensureCodecStarted(state); started.is_err()) {
    return started;
  }

  while (availableFrames(state.nativeLeftover, state.nativeCursor, state.channels) == 0 &&
         !state.outputEnded) {
    pumpCodec(state);
  }
  return Ok(None);
}

decoding::DecoderResult attachMemoryExtractor(
    AndroidDecoderState &state,
    std::vector<uint8_t> data) {
  state.extractor.reset(AMediaExtractor_new());
  if (state.extractor == nullptr) {
    return Err("AndroidDecoder::open: AMediaExtractor_new failed");
  }

  AMediaExtractor *extractor = state.extractor.get();
  if (android_get_device_api_level() >= kMediaDataSourceMinApiLevel) {
    return attachMemoryExtractorViaDataSource(
        extractor,
        state.dataSource,
        state.encodedMemory,
        state.memorySourceContext,
        std::move(data));
  }

  return attachMemoryExtractorViaTempFile(extractor, state.tempFile, data);
}

} // namespace

AndroidDecoder::AndroidDecoder() = default;

AndroidDecoder::~AndroidDecoder() {
  close();
}

decoding::DecoderResult AndroidDecoder::open(const decoding::LocalFileSource &source) {
  close();
  if (source.path.empty()) {
    return Err("AndroidDecoder::open failed: path is empty");
  }

  auto state = std::make_unique<AndroidDecoderState>();
  state->extractor.reset(AMediaExtractor_new());
  if (state->extractor == nullptr) {
    return Err("AndroidDecoder::open: AMediaExtractor_new failed");
  }

  if (AMediaExtractor_setDataSource(state->extractor.get(), source.path.c_str()) != AMEDIA_OK) {
    return Err("AndroidDecoder::open setDataSource failed");
  }

  return finishOpeningDecoder(std::move(state), source.sampleRate);
}

decoding::DecoderResult AndroidDecoder::open(const decoding::EncodedMemorySource &source) {
  close();
  if (source.data.empty()) {
    return Err("AndroidDecoder::open failed: empty input");
  }

  auto state = std::make_unique<AndroidDecoderState>();

  if (auto attached = attachMemoryExtractor(*state, source.data); attached.is_err()) {
    return attached;
  }

  return finishOpeningDecoder(std::move(state), source.sampleRate);
}

decoding::DecoderResult AndroidDecoder::finishOpeningDecoder(
    std::unique_ptr<AndroidDecoderState> state,
    int sampleRate) {
  if (auto configured = configureExtractorMetadata(*state, sampleRate); configured.is_err()) {
    return configured;
  }

  if (auto settled = settleCodecOutputFormat(*state); settled.is_err()) {
    return settled;
  }

  outputChannels_ = state->channels;
  outputSampleRate_ = state->outputRate;
  framePosition_ = 0;
  setTotalPcmFramesFromDuration(state->durationSeconds);
  impl_ = std::move(state);
  open_ = true;
  return Ok(None);
}

size_t AndroidDecoder::readPcmFrames(float *outInterleaved, size_t frameCount) {
  if (!open_ || impl_ == nullptr || outInterleaved == nullptr || frameCount == 0 ||
      outputChannels_ <= 0) {
    return 0;
  }

  auto &state = *impl_;
  if (ensureCodecStarted(state).is_err()) {
    return 0;
  }

  size_t filled = 0; // frames
  // The caller sized outInterleaved from outputChannels(), so this call must
  // never write wider frames than that. open() settles the codec format up
  // front, so a change here is anomalous — stop rather than overflow, and keep
  // the published layout stable for the frames already written.
  const int callerChannels = outputChannels_;

  while (filled < frameCount) {
    if (state.channels != callerChannels) {
      break;
    }
    outputSampleRate_ = state.outputRate;

    const bool resample = state.resampler != nullptr;
    std::vector<float> &served = resample ? state.outLeftover : state.nativeLeftover;
    size_t &cursor = resample ? state.outCursor : state.nativeCursor;

    const size_t avail = availableFrames(served, cursor, callerChannels);
    if (avail > 0) {
      const size_t take = std::min(avail, frameCount - filled);
      const size_t samples = take * static_cast<size_t>(callerChannels);
      std::memcpy(
          outInterleaved + filled * static_cast<size_t>(callerChannels),
          served.data() + cursor,
          samples * sizeof(float));
      cursor += samples;
      filled += take;
      continue;
    }

    if (resample) {
      if (produceResampled(state) == 0) {
        break; // EOF: native exhausted and codec ended.
      }
    } else {
      if (state.outputEnded) {
        break;
      }
      pumpCodec(state);
    }
  }

  framePosition_ += static_cast<int64_t>(filled);
  return filled;
}

decoding::DecoderResult AndroidDecoder::seekToTime(double seconds) {
  if (!open_ || impl_ == nullptr || impl_->extractor == nullptr) {
    return Err("AndroidDecoder::seekToTime failed: decoder is not open");
  }
  if (!std::isfinite(seconds) || seconds < 0.0) {
    return Err("AndroidDecoder::seekToTime failed: seconds is not finite");
  }

  auto &state = *impl_;
  if (auto started = ensureCodecStarted(state); started.is_err()) {
    return started;
  }

  const auto seekUs = static_cast<int64_t>(std::llround(seconds * 1e6));
  if (AMediaExtractor_seekTo(state.extractor.get(), seekUs, AMEDIAEXTRACTOR_SEEK_CLOSEST_SYNC) !=
      AMEDIA_OK) {
    return Err("AndroidDecoder::seekToTime: AMediaExtractor_seekTo failed");
  }
  AMediaCodec_flush(state.codec.get());

  state.nativeLeftover.clear();
  state.nativeCursor = 0;
  state.outLeftover.clear();
  state.outCursor = 0;
  state.inputEnded = false;
  state.outputEnded = false;
  state.tryAgainAfterEos = 0;

  // Reset resampler state so pre-seek samples don't smear into the new position.
  if (state.resampler != nullptr) {
    setupResamplerIfNeeded(state);
  }

  framePosition_ =
      static_cast<int64_t>(std::llround(seconds * static_cast<double>(outputSampleRate_)));
  return Ok(None);
}

void AndroidDecoder::releaseImpl() {
  impl_.reset();
}

} // namespace audioapi::android_decoder
