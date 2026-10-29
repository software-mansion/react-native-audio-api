#include <audioapi/encoding/OggOpusEncoder.h>

#if !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED

#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/encoding/EncoderOutputSpec.h>
#include <audioapi/encoding/OggStreamWriter.h>
#include <audioapi/external/include/opus/opus.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioFileProperties.h>
#include <audioapi/utils/FileSystem.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/UnitConversion.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace audioapi::ogg_opus {

namespace {

constexpr int OPUS_SAMPLE_RATE = 48000;
/// 20 ms, the Opus frame size with the best quality/overhead balance (RFC 6716 §2.1.4).
constexpr int FRAMES_PER_PACKET = 960;
constexpr opus_int32 DEFAULT_BIT_RATE = 96000;
/// The range OPUS_SET_BITRATE accepts; anything outside it is rejected, not clamped, by libopus.
constexpr opus_int32 MIN_BIT_RATE = 500;
constexpr opus_int32 MAX_BIT_RATE = 512000;
/// libopus never needs more than 1275 bytes for a 20 ms frame; its docs recommend 4000.
constexpr size_t MAX_PACKET_BYTES = 4000;
/// Pages are forced out once a second, so a recording cut short stays playable up to the last
/// completed second (RFC 7845 §4 recommends at most one second of audio per page).
constexpr int PACKETS_PER_FORCED_PAGE = 50;
constexpr int RESAMPLE_MAX_IN_FRAMES = 4096;
/// Zero frames pushed per step while draining the resampler's group delay at close().
constexpr int RESAMPLER_DRAIN_STEP_FRAMES = 256;
/// Bounds the drain loop; r8brain's delay is a few hundred frames, far below this.
constexpr int RESAMPLER_DRAIN_MAX_STEPS = 64;

constexpr size_t OPUS_HEAD_BYTES = 19;
constexpr unsigned char OPUS_HEAD_VERSION = 1;
/// RFC 7845 §5.1.1: family 0 is mono or stereo with no explicit channel mapping table.
constexpr unsigned char CHANNEL_MAPPING_FAMILY_RTP = 0;

constexpr unsigned int BITS_PER_BYTE = 8;
constexpr unsigned int BYTE_MASK = 0xFFU;

} // namespace

struct EncoderState {
  EncoderState() = default;
  DELETE_COPY_AND_MOVE(EncoderState);

  ::OpusEncoder *opus = nullptr;
  OggStreamWriter ogg;

  int channelCount = 0;
  /// Encoder lookahead in 48 kHz frames; a decoder discards this many frames at the start.
  int preSkip = 0;
  /// 48 kHz frames the Opus encoder has coded, padding included.
  int64_t encodedFrames = 0;
  /// 48 kHz frames of converted input, which is what a decoder must give back.
  int64_t inputFramesAt48k = 0;
  int packetsSinceForcedPage = 0;

  /// One packet of interleaved frames; the unused tail is silence.
  std::vector<float> pendingInterleaved;
  int pendingFrames = 0;
  std::vector<unsigned char> packet;
  std::vector<float> silence;

  // Conversion to the file layout, rebuilt per input format.
  size_t maxInputFrames = 0;
  float inputSampleRate = 0.0F;
  /// Channel mapping only: the input is staged in inputPlanar and remixed into mappedPlanar.
  std::unique_ptr<AudioBuffer> inputPlanar;
  std::unique_ptr<AudioBuffer> mappedPlanar;
  /// Rate conversion only.
  std::unique_ptr<r8b::MultiChannelResampler> resampler;
  std::unique_ptr<AudioBuffer> resampledPlanar;
  int64_t resamplerInputFrames = 0;
  int64_t resamplerOutputFrames = 0;

  ~EncoderState() {
    release();
  }

  void release() {
    if (opus != nullptr) {
      opus_encoder_destroy(opus);
      opus = nullptr;
    }
    ogg.close();
    inputPlanar.reset();
    mappedPlanar.reset();
    resampler.reset();
    resampledPlanar.reset();
  }
};

namespace {

/// Little-endian, as every multi-byte field of the Opus headers is.
void appendLittleEndian(std::vector<unsigned char> &out, uint32_t value, unsigned int byteCount) {
  for (unsigned int byte = 0; byte < byteCount; ++byte) {
    out.push_back(static_cast<unsigned char>((value >> (byte * BITS_PER_BYTE)) & BYTE_MASK));
  }
}

void appendLe16(std::vector<unsigned char> &out, uint16_t value) {
  appendLittleEndian(out, value, sizeof(uint16_t));
}

void appendLe32(std::vector<unsigned char> &out, uint32_t value) {
  appendLittleEndian(out, value, sizeof(uint32_t));
}

/// RFC 7845 §5.1. The input sample rate field only records the rate before encoding; players
/// decode at 48 kHz regardless, and the file is written at that rate.
std::vector<unsigned char> opusHead(int channelCount, int preSkip) {
  std::vector<unsigned char> head;
  head.reserve(OPUS_HEAD_BYTES);
  head.insert(head.end(), {'O', 'p', 'u', 's', 'H', 'e', 'a', 'd'});
  head.push_back(OPUS_HEAD_VERSION);
  head.push_back(static_cast<unsigned char>(channelCount));
  appendLe16(head, static_cast<uint16_t>(preSkip));
  appendLe32(head, static_cast<uint32_t>(OPUS_SAMPLE_RATE));
  appendLe16(head, 0); // output gain, Q7.8 dB
  head.push_back(CHANNEL_MAPPING_FAMILY_RTP);
  return head;
}

/// RFC 7845 §5.2, with the libopus version as the vendor string and no user comments.
std::vector<unsigned char> opusTags() {
  const std::string vendor = opus_get_version_string();
  std::vector<unsigned char> tags;
  tags.insert(tags.end(), {'O', 'p', 'u', 's', 'T', 'a', 'g', 's'});
  appendLe32(tags, static_cast<uint32_t>(vendor.size()));
  tags.insert(tags.end(), vendor.begin(), vendor.end());
  appendLe32(tags, 0);
  return tags;
}

/// Each header packet gets a page of its own (RFC 7845 §3).
Result<NoneType, std::string> writeHeaders(EncoderState &state) {
  const auto head = opusHead(state.channelCount, state.preSkip);
  auto headResult = state.ogg.submit(head.data(), head.size(), 0, true, false);
  if (headResult.is_err()) {
    return headResult;
  }
  auto headPages = state.ogg.writePages(true);
  if (headPages.is_err()) {
    return headPages;
  }

  const auto tags = opusTags();
  auto tagsResult = state.ogg.submit(tags.data(), tags.size(), 0, false, false);
  if (tagsResult.is_err()) {
    return tagsResult;
  }
  return state.ogg.writePages(true);
}

/// Codes the pending packet; its silent tail must already be zeroed when it is partial.
Result<NoneType, std::string>
encodePendingPacket(EncoderState &state, int64_t granulePosition, bool endOfStream) {
  const int bytes = opus_encode_float(
      state.opus,
      state.pendingInterleaved.data(),
      FRAMES_PER_PACKET,
      state.packet.data(),
      static_cast<opus_int32>(state.packet.size()));
  if (bytes < 0) {
    return Err(std::string("Opus encoding failed: ") + opus_strerror(bytes));
  }
  state.encodedFrames += FRAMES_PER_PACKET;
  state.pendingFrames = 0;

  auto submitResult = state.ogg.submit(
      state.packet.data(), static_cast<size_t>(bytes), granulePosition, false, endOfStream);
  if (submitResult.is_err()) {
    return submitResult;
  }

  ++state.packetsSinceForcedPage;
  const bool forcePage = endOfStream || state.packetsSinceForcedPage >= PACKETS_PER_FORCED_PAGE;
  if (forcePage) {
    state.packetsSinceForcedPage = 0;
  }
  auto pagesResult = state.ogg.writePages(forcePage);
  if (pagesResult.is_err()) {
    return pagesResult;
  }
  return forcePage ? state.ogg.flush() : Ok(None);
}

/// Interleaves 48 kHz frames in the file layout into packets, coding each one as it fills.
Result<NoneType, std::string>
queueFrames(EncoderState &state, const float *const *planar, int numFrames) {
  const int channelCount = state.channelCount;
  int consumed = 0;
  while (consumed < numFrames) {
    const int take = std::min(FRAMES_PER_PACKET - state.pendingFrames, numFrames - consumed);
    float *destination =
        state.pendingInterleaved.data() + static_cast<size_t>(state.pendingFrames) * channelCount;
    for (int frame = 0; frame < take; ++frame) {
      for (int channel = 0; channel < channelCount; ++channel) {
        destination[frame * channelCount + channel] = planar[channel][consumed + frame];
      }
    }
    state.pendingFrames += take;
    consumed += take;
    state.inputFramesAt48k += take;

    if (state.pendingFrames == FRAMES_PER_PACKET) {
      auto result = encodePendingPacket(state, state.encodedFrames + FRAMES_PER_PACKET, false);
      if (result.is_err()) {
        return result;
      }
    }
  }
  return Ok(None);
}

/// r8brain compensates its latency by holding back a group delay's worth of output, so the
/// last input frames only come out once silence is pushed after them.
Result<NoneType, std::string> drainResampler(EncoderState &state) {
  if (state.resampler == nullptr) {
    return Ok(None);
  }
  const auto expectedOutputFrames = static_cast<int64_t>(std::llround(
      static_cast<double>(state.resamplerInputFrames) * OPUS_SAMPLE_RATE / state.inputSampleRate));

  std::array<const float *, MAX_CHANNEL_COUNT> zeros{};
  std::array<float *, MAX_CHANNEL_COUNT> resampled{};
  for (int channel = 0; channel < state.channelCount; ++channel) {
    zeros[channel] = state.silence.data();
    resampled[channel] = state.resampledPlanar->getChannel(channel)->begin();
  }

  for (int step = 0;
       step < RESAMPLER_DRAIN_MAX_STEPS && state.resamplerOutputFrames < expectedOutputFrames;
       ++step) {
    const int produced =
        state.resampler->process(zeros.data(), RESAMPLER_DRAIN_STEP_FRAMES, resampled.data());
    if (produced <= 0) {
      continue;
    }
    const auto take = static_cast<int>(
        std::min<int64_t>(produced, expectedOutputFrames - state.resamplerOutputFrames));
    state.resamplerOutputFrames += produced;
    auto result = queueFrames(state, resampled.data(), take);
    if (result.is_err()) {
      return result;
    }
  }
  return Ok(None);
}

/// Pads the stream with silence until a decoder's output covers every input frame after
/// pre-skip, then trims the surplus through the last granule position (RFC 7845 §4.4).
Result<NoneType, std::string> finishStream(EncoderState &state) {
  auto drainResult = drainResampler(state);
  if (drainResult.is_err()) {
    return drainResult;
  }

  const int64_t endGranulePosition = state.inputFramesAt48k + state.preSkip;
  while (true) {
    std::fill(
        state.pendingInterleaved.begin() +
            static_cast<std::ptrdiff_t>(state.pendingFrames) * state.channelCount,
        state.pendingInterleaved.end(),
        0.0F);
    const bool last = state.encodedFrames + FRAMES_PER_PACKET >= endGranulePosition;
    auto result = encodePendingPacket(
        state, last ? endGranulePosition : state.encodedFrames + FRAMES_PER_PACKET, last);
    if (result.is_err() || last) {
      return result;
    }
  }
}

} // namespace

OggOpusEncoder::OggOpusEncoder(const EncoderSettings &settings)
    : AudioEncoder(settings), state_(std::make_unique<EncoderState>()) {}

OggOpusEncoder::~OggOpusEncoder() {
  if (isOpen()) {
    close();
  }
}

OpenEncoderResult OggOpusEncoder::open(
    const StreamFormat &inputFormat,
    const EncoderOutputSpec &outputSpec,
    const std::string &filePath) {
  if (isOpen()) {
    return Err("Encoder already open");
  }
  if (outputSpec.container != AudioContainer::OGG || outputSpec.codec != AudioCodec::OPUS) {
    return Err(
        std::string("The Ogg/Opus encoder cannot write ") + toString(outputSpec.codec) + " in " +
        toString(outputSpec.container));
  }
  if (inputFormat.layout.sampleRate <= 0 || inputFormat.layout.channelCount <= 0) {
    return Err("Invalid input format: sampleRate and channelCount must be greater than 0");
  }
  if (settings_.fileLayout.channelCount < 1 ||
      settings_.fileLayout.channelCount > AudioFileProperties::MAX_FILE_CHANNEL_COUNT) {
    return Err("Opus files are mono or stereo");
  }

  outputSpec_ = outputSpec;
  filePath_ = filePath;
  resetEncodedDuration();

  EncoderState &state = *state_;
  state.release();
  state.channelCount = settings_.fileLayout.channelCount;
  state.encodedFrames = 0;
  state.inputFramesAt48k = 0;
  state.packetsSinceForcedPage = 0;
  state.pendingFrames = 0;
  state.pendingInterleaved.assign(
      static_cast<size_t>(FRAMES_PER_PACKET) * state.channelCount, 0.0F);
  state.packet.assign(MAX_PACKET_BYTES, 0);
  state.silence.assign(RESAMPLER_DRAIN_STEP_FRAMES, 0.0F);

  int opusError = OPUS_OK;
  state.opus =
      opus_encoder_create(OPUS_SAMPLE_RATE, state.channelCount, OPUS_APPLICATION_AUDIO, &opusError);
  if (state.opus == nullptr || opusError != OPUS_OK) {
    state.release();
    return Err(std::string("Failed to create the Opus encoder: ") + opus_strerror(opusError));
  }

  const opus_int32 requestedBitRate = settings_.encoding.bitRate > 0
      ? static_cast<opus_int32>(
            std::min(settings_.encoding.bitRate, static_cast<size_t>(MAX_BIT_RATE)))
      : DEFAULT_BIT_RATE;
  const opus_int32 bitRate = std::clamp(requestedBitRate, MIN_BIT_RATE, MAX_BIT_RATE);
  if (opus_encoder_ctl(state.opus, OPUS_SET_BITRATE(bitRate)) != OPUS_OK) {
    state.release();
    return Err("Failed to set the Opus bit rate to " + std::to_string(bitRate));
  }
  opus_int32 lookahead = 0;
  if (opus_encoder_ctl(state.opus, OPUS_GET_LOOKAHEAD(&lookahead)) != OPUS_OK) {
    state.release();
    return Err("Failed to read the Opus encoder lookahead");
  }
  state.preSkip = lookahead;

  auto streamResult = state.ogg.open(filePath);
  if (streamResult.is_err()) {
    discardOutputFile();
    return Err(streamResult.unwrap_err());
  }

  auto headersResult = writeHeaders(state);
  if (headersResult.is_err()) {
    discardOutputFile();
    return Err(headersResult.unwrap_err());
  }

  auto conversionResult = prepareConversion(inputFormat);
  if (conversionResult.is_err()) {
    discardOutputFile();
    return Err(conversionResult.unwrap_err());
  }

  markOpen();
  return Ok(filePath_);
}

OpenEncoderResult OggOpusEncoder::reprepareInput(const StreamFormat &inputFormat) {
  if (!isOpen()) {
    return Err("Encoder is not open");
  }
  if (inputFormat.layout.sampleRate <= 0 || inputFormat.layout.channelCount <= 0) {
    return Err("Invalid input format: sampleRate and channelCount must be greater than 0");
  }
  auto conversionResult = prepareConversion(inputFormat);
  if (conversionResult.is_err()) {
    return Err(conversionResult.unwrap_err());
  }
  return Ok(filePath_);
}

Result<NoneType, std::string> OggOpusEncoder::prepareConversion(const StreamFormat &inputFormat) {
  const AudioLayout &inputLayout = inputFormat.layout;
  if (inputLayout.channelCount > MAX_CHANNEL_COUNT) {
    return Err("Channel count exceeds MAX_CHANNEL_COUNT");
  }

  EncoderState &state = *state_;
  state.inputPlanar.reset();
  state.mappedPlanar.reset();
  state.resampler.reset();
  state.resampledPlanar.reset();
  state.maxInputFrames = std::max<size_t>(inputFormat.maxFramesPerBuffer, 1);
  state.inputSampleRate = inputLayout.sampleRate;
  state.resamplerInputFrames = 0;
  state.resamplerOutputFrames = 0;

  if (inputLayout.channelCount != state.channelCount) {
    state.inputPlanar = std::make_unique<AudioBuffer>(
        state.maxInputFrames, inputLayout.channelCount, inputLayout.sampleRate);
    state.mappedPlanar = std::make_unique<AudioBuffer>(
        state.maxInputFrames, state.channelCount, inputLayout.sampleRate);
  }
  if (inputLayout.sampleRate != static_cast<float>(OPUS_SAMPLE_RATE)) {
    state.resampler = std::make_unique<r8b::MultiChannelResampler>(
        inputLayout.sampleRate, OPUS_SAMPLE_RATE, state.channelCount, RESAMPLE_MAX_IN_FRAMES);
    state.resampledPlanar = std::make_unique<AudioBuffer>(
        static_cast<size_t>(std::max(1, state.resampler->getMaxOutLen())),
        state.channelCount,
        static_cast<float>(OPUS_SAMPLE_RATE));
  }

  inputFormat_ = inputFormat;
  return Ok(None);
}

Result<NoneType, std::string> OggOpusEncoder::convertAndQueue(
    const float *const *channels,
    int numFrames) {
  EncoderState &state = *state_;
  const auto frames = static_cast<size_t>(numFrames);

  std::array<const float *, MAX_CHANNEL_COUNT> planar{};
  if (state.mappedPlanar != nullptr) {
    for (int channel = 0; channel < inputFormat_.layout.channelCount; ++channel) {
      std::memcpy(
          state.inputPlanar->getChannel(channel)->begin(),
          channels[channel],
          frames * sizeof(float));
    }
    state.mappedPlanar->copy(*state.inputPlanar, 0, 0, frames);
    for (int channel = 0; channel < state.channelCount; ++channel) {
      planar[channel] = state.mappedPlanar->getChannel(channel)->begin();
    }
  } else {
    for (int channel = 0; channel < state.channelCount; ++channel) {
      planar[channel] = channels[channel];
    }
  }

  if (state.resampler == nullptr) {
    return queueFrames(state, planar.data(), numFrames);
  }

  std::array<float *, MAX_CHANNEL_COUNT> resampled{};
  for (int channel = 0; channel < state.channelCount; ++channel) {
    resampled[channel] = state.resampledPlanar->getChannel(channel)->begin();
  }

  int consumed = 0;
  while (consumed < numFrames) {
    const int chunk = std::min(numFrames - consumed, RESAMPLE_MAX_IN_FRAMES);
    std::array<const float *, MAX_CHANNEL_COUNT> chunkChannels{};
    for (int channel = 0; channel < state.channelCount; ++channel) {
      chunkChannels[channel] = planar[channel] + consumed;
    }
    const int produced = state.resampler->process(chunkChannels.data(), chunk, resampled.data());
    consumed += chunk;
    state.resamplerInputFrames += chunk;
    if (produced <= 0) {
      continue;
    }
    state.resamplerOutputFrames += produced;
    auto result = queueFrames(state, resampled.data(), produced);
    if (result.is_err()) {
      return result;
    }
  }
  return Ok(None);
}

EncodeResult OggOpusEncoder::encode(const float *const *channels, int numFrames) {
  if (!isOpen()) {
    return Err("Encoder is not open");
  }
  if (channels == nullptr || numFrames <= 0) {
    return Err("Invalid encode input");
  }
  if (static_cast<size_t>(numFrames) > state_->maxInputFrames) {
    return Err("Encode input exceeds the buffer size declared at open()");
  }

  auto result = convertAndQueue(channels, numFrames);
  if (result.is_err()) {
    return Err(result.unwrap_err());
  }
  addEncodedFrames(static_cast<size_t>(numFrames));
  return Ok(static_cast<size_t>(numFrames));
}

CloseEncoderResult OggOpusEncoder::close() {
  if (!isOpen()) {
    return Err("Encoder is not open");
  }
  markClosed();

  EncoderState &state = *state_;
  auto finishResult = finishStream(state);
  if (finishResult.is_ok()) {
    finishResult = state.ogg.flush();
  }
  const std::string error = finishResult.is_err() ? finishResult.unwrap_err() : "";

  const size_t sizeBytes = state.ogg.bytesWritten();
  const double durationSeconds = getEncodedDurationSeconds();
  state.release();
  resetEncodedDuration();

  if (!error.empty()) {
    return Err(error);
  }
  return Ok(std::make_tuple(static_cast<double>(sizeBytes) / MB_IN_BYTES, durationSeconds));
}

size_t OggOpusEncoder::getFileSizeBytes() const {
  return state_->ogg.bytesWritten();
}

/// For a failed open(): a header-only file at the requested path would pass for a recording.
void OggOpusEncoder::discardOutputFile() {
  state_->release();
  file_system::removeFile(filePath_);
}

} // namespace audioapi::ogg_opus

#endif // !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
