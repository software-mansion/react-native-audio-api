#include <audioapi/core/utils/AudioFileConcatenator.h>
#include <audioapi/libs/miniaudio/miniaudio.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if !RN_AUDIO_API_FFMPEG_DISABLED
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/dict.h>
#include <libavutil/mathematics.h>
}
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace audioapi {

#if !RN_AUDIO_API_FFMPEG_DISABLED
AVDictionaryGuard::~AVDictionaryGuard() {
  av_dict_free(&dictionary_);
}

AVDictionary **AVDictionaryGuard::ptr() {
  return &dictionary_;
}

InputFormatContext::InputFormatContext(
    std::string filePath,
    AVFormatContext *context,
    int audioStreamIndex)
    : filePath_(std::move(filePath)), context_(context), audioStreamIndex_(audioStreamIndex) {}

InputFormatContext::InputFormatContext(InputFormatContext &&other) noexcept
    : filePath_(std::move(other.filePath_)),
      context_(std::exchange(other.context_, nullptr)),
      audioStreamIndex_(std::exchange(other.audioStreamIndex_, -1)) {}

InputFormatContext &InputFormatContext::operator=(InputFormatContext &&other) noexcept {
  if (this != &other) {
    close();
    filePath_ = std::move(other.filePath_);
    context_ = std::exchange(other.context_, nullptr);
    audioStreamIndex_ = std::exchange(other.audioStreamIndex_, -1);
  }
  return *this;
}

InputFormatContext::~InputFormatContext() {
  close();
}

AVFormatContext *InputFormatContext::context() const {
  return context_;
}

AVStream *InputFormatContext::audioStream() const {
  return context_->streams[audioStreamIndex_];
}

int InputFormatContext::audioStreamIndex() const {
  return audioStreamIndex_;
}

const std::string &InputFormatContext::filePath() const {
  return filePath_;
}

void InputFormatContext::close() {
  if (context_ != nullptr) {
    avformat_close_input(&context_);
  }
}

OutputFormatContext::OutputFormatContext(AVFormatContext *context) : context_(context) {}

OutputFormatContext::~OutputFormatContext() {
  if (context_ == nullptr) {
    return;
  }

  if (!(context_->oformat->flags & AVFMT_NOFILE) && context_->pb != nullptr) {
    avio_closep(&context_->pb);
  }

  avformat_free_context(context_);
}

AVFormatContext *OutputFormatContext::get() const {
  return context_;
}
#endif // RN_AUDIO_API_FFMPEG_DISABLED

namespace {

constexpr const char *fileUrlPrefix = "file://";
constexpr ma_uint64 miniaudioChunkFrames = 4096;
constexpr ma_uint64 riffWaveHeaderBytes = 36;
constexpr ma_uint64 maxRiffChunkSize = std::numeric_limits<uint32_t>::max();

bool hasNonFileProtocol(const std::string &path) {
  const auto colon = path.find(':');
  if (colon == std::string::npos) {
    return false;
  }

  const auto firstSlash = path.find('/');
  return firstSlash == std::string::npos || colon < firstSlash;
}

std::string percentDecode(const std::string &path) {
  std::string decoded;
  decoded.reserve(path.size());

  for (size_t i = 0; i < path.size(); ++i) {
    if (path[i] != '%' || i + 2 >= path.size()) {
      decoded.push_back(path[i]);
      continue;
    }

    unsigned int value = 0;
    auto first = path.data() + i + 1;
    auto last = first + 2;
    auto result = std::from_chars(first, last, value, 16);
    if (result.ec != std::errc() || result.ptr != last) {
      decoded.push_back(path[i]);
      continue;
    }

    decoded.push_back(static_cast<char>(value));
    i += 2;
  }

  return decoded;
}

AudioFileConcatResult validatePaths(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  if (inputPaths.empty()) {
    return Err("concatAudioFiles requires at least one input path.");
  }

  for (size_t i = 0; i < inputPaths.size(); ++i) {
    if (inputPaths[i].empty()) {
      return Err("concatAudioFiles input path at index " + std::to_string(i) + " is empty.");
    }

    if (hasNonFileProtocol(inputPaths[i])) {
      return Err(
          "concatAudioFiles input path at index " + std::to_string(i) +
          " must be a local file path or file:// URL.");
    }
  }

  if (outputPath.empty()) {
    return Err("concatAudioFiles requires an output path.");
  }

  if (hasNonFileProtocol(outputPath)) {
    return Err("concatAudioFiles output path must be a local file path or file:// URL.");
  }

  return Ok(outputPath);
}

std::string lowercaseExtension(const std::string &path) {
  const auto dotIndex = path.find_last_of('.');
  if (dotIndex == std::string::npos) {
    return "";
  }

  std::string extension = path.substr(dotIndex + 1);
  std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  return extension;
}

bool hasExtension(const std::string &path, const std::vector<std::string> &extensions) {
  const std::string extension = lowercaseExtension(path);
  return std::ranges::find(extensions, extension) != extensions.end();
}

bool isMiniaudioOutputPath(const std::string &path) {
  return hasExtension(path, {"wav"});
}

bool isFFmpegRemuxOutputPath(const std::string &path) {
  return hasExtension(path, {"m4a", "mp4"});
}

bool isFFmpegOutputPath(const std::string &path) {
  return isFFmpegRemuxOutputPath(path);
}

std::string parseMiniAudioError(ma_result errorCode) {
  return {ma_result_description(errorCode)};
}

} // namespace

MiniAudioDecoderGuard::MiniAudioDecoderGuard(MiniAudioDecoderGuard &&other) noexcept
    : filePath_(std::move(other.filePath_)),
      decoder_(std::move(other.decoder_)),
      initialized_(std::exchange(other.initialized_, false)) {}

MiniAudioDecoderGuard &MiniAudioDecoderGuard::operator=(MiniAudioDecoderGuard &&other) noexcept {
  if (this != &other) {
    close();
    filePath_ = std::move(other.filePath_);
    decoder_ = std::move(other.decoder_);
    initialized_ = std::exchange(other.initialized_, false);
  }
  return *this;
}

MiniAudioDecoderGuard::~MiniAudioDecoderGuard() {
  close();
}

Result<MiniAudioDecoderGuard, std::string> MiniAudioDecoderGuard::open(
    const std::string &filePath,
    ma_format outputFormat) {
  MiniAudioDecoderGuard input;
  input.filePath_ = filePath;
  input.decoder_ = std::make_unique<ma_decoder>();

  ma_decoder_config config = ma_decoder_config_init(outputFormat, 0, 0);
  const ma_result result = ma_decoder_init_file(filePath.c_str(), &config, input.decoder_.get());
  if (result != MA_SUCCESS) {
    return Err(
        "Failed to open input file '" + filePath +
        "' with miniaudio: " + parseMiniAudioError(result));
  }

  input.initialized_ = true;
  if (input.decoder_->outputSampleRate == 0 || input.decoder_->outputChannels == 0) {
    return Err("Input file '" + filePath + "' is missing required audio parameters.");
  }

  return Ok(std::move(input));
}

ma_decoder *MiniAudioDecoderGuard::get() {
  return decoder_.get();
}

const std::string &MiniAudioDecoderGuard::filePath() const {
  return filePath_;
}

ma_uint32 MiniAudioDecoderGuard::sampleRate() const {
  return decoder_->outputSampleRate;
}

ma_uint32 MiniAudioDecoderGuard::channels() const {
  return decoder_->outputChannels;
}

ma_format MiniAudioDecoderGuard::format() const {
  return decoder_->outputFormat;
}

void MiniAudioDecoderGuard::close() {
  if (initialized_ && decoder_ != nullptr) {
    ma_decoder_uninit(decoder_.get());
    initialized_ = false;
  }
  decoder_.reset();
}

MiniAudioEncoderGuard::~MiniAudioEncoderGuard() {
  close();
}

AudioFileConcatResult MiniAudioEncoderGuard::open(
    const std::string &outputPath,
    ma_format format,
    ma_uint32 sampleRate,
    ma_uint32 channels) {
  ma_encoder_config config =
      ma_encoder_config_init(ma_encoding_format_wav, format, channels, sampleRate);

  const ma_result result = ma_encoder_init_file(outputPath.c_str(), &config, &encoder_);
  if (result != MA_SUCCESS) {
    return Err(
        "Failed to open output file '" + outputPath +
        "' with miniaudio: " + parseMiniAudioError(result));
  }

  initialized_ = true;
  return Ok(outputPath);
}

AudioFileConcatResult MiniAudioEncoderGuard::write(
    const std::string &inputPath,
    const void *frames,
    ma_uint64 frameCount) {
  ma_uint64 framesWritten = 0;
  const ma_result result =
      ma_encoder_write_pcm_frames(&encoder_, frames, frameCount, &framesWritten);
  if (result != MA_SUCCESS) {
    return Err(
        "Failed to write decoded frames from '" + inputPath +
        "' with miniaudio: " + parseMiniAudioError(result));
  }

  if (framesWritten != frameCount) {
    return Err("Failed to write all decoded frames from '" + inputPath + "' with miniaudio.");
  }

  return Ok(inputPath);
}

void MiniAudioEncoderGuard::close() {
  if (initialized_) {
    ma_encoder_uninit(&encoder_);
    initialized_ = false;
  }
}

namespace {

AudioFileConcatResult validateCompatibleMiniAudioInput(
    const MiniAudioDecoderGuard &input,
    const MiniAudioDecoderGuard &reference) {
  if (input.sampleRate() != reference.sampleRate()) {
    return Err("Input file '" + input.filePath() + "' uses a different sample rate.");
  }

  if (input.channels() != reference.channels()) {
    return Err("Input file '" + input.filePath() + "' uses a different channel count.");
  }

  if (input.format() != reference.format()) {
    return Err("Input file '" + input.filePath() + "' uses a different sample format.");
  }

  return Ok(input.filePath());
}

AudioFileConcatResult openAndValidateMiniAudioInputs(
    const std::vector<std::string> &inputPaths,
    std::vector<MiniAudioDecoderGuard> &inputs,
    ma_format outputFormat = ma_format_unknown) {
  inputs.reserve(inputPaths.size());

  for (const auto &inputPath : inputPaths) {
    auto inputResult = MiniAudioDecoderGuard::open(inputPath, outputFormat);
    if (inputResult.is_err()) {
      return Err(inputResult.unwrap_err());
    }

    inputs.emplace_back(std::move(inputResult).unwrap());

    if (inputs.size() > 1) {
      auto validationResult = validateCompatibleMiniAudioInput(inputs.back(), inputs.front());
      if (validationResult.is_err()) {
        return validationResult;
      }
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult validateRiffWaveOutputSize(
    std::vector<MiniAudioDecoderGuard> &inputs,
    ma_uint64 bytesPerFrame) {
  if (bytesPerFrame == 0) {
    return Err("Input files use an unsupported WAV sample format.");
  }

  ma_uint64 totalDataBytes = 0;
  for (auto &input : inputs) {
    ma_uint64 frameCount = 0;
    const ma_result result = ma_decoder_get_length_in_pcm_frames(input.get(), &frameCount);
    if (result != MA_SUCCESS) {
      return Err(
          "Failed to determine decoded frame count for '" + input.filePath() +
          "' with miniaudio: " + parseMiniAudioError(result));
    }

    if (frameCount > (std::numeric_limits<ma_uint64>::max() - totalDataBytes) / bytesPerFrame) {
      return Err(
          "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");
    }

    totalDataBytes += frameCount * bytesPerFrame;
    if (riffWaveHeaderBytes + totalDataBytes + (totalDataBytes & 1) > maxRiffChunkSize) {
      return Err(
          "concatAudioFiles WAV output exceeds the RIFF WAV size limit. Split the output or use a format with RF64/W64 support.");
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult concatAudioFilesWithMiniAudio(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  if (!isMiniaudioOutputPath(outputPath)) {
    return Err("concatAudioFiles supports miniaudio output only for WAV files.");
  }

  for (const auto &inputPath : inputPaths) {
    if (!hasExtension(inputPath, {"wav"})) {
      return Err("concatAudioFiles WAV output requires all input files to use the WAV extension.");
    }
  }

  std::vector<MiniAudioDecoderGuard> inputs;
  auto inputValidationResult = openAndValidateMiniAudioInputs(inputPaths, inputs);
  if (inputValidationResult.is_err()) {
    return inputValidationResult;
  }

  const ma_uint64 bytesPerFrame =
      inputs.front().channels() * ma_get_bytes_per_sample(inputs.front().format());
  auto outputSizeResult = validateRiffWaveOutputSize(inputs, bytesPerFrame);
  if (outputSizeResult.is_err()) {
    return outputSizeResult;
  }

  MiniAudioEncoderGuard output;
  auto outputResult = output.open(
      outputPath, inputs.front().format(), inputs.front().sampleRate(), inputs.front().channels());
  if (outputResult.is_err()) {
    return outputResult;
  }

  std::vector<uint8_t> buffer(miniaudioChunkFrames * bytesPerFrame);
  for (auto &input : inputs) {
    while (true) {
      ma_uint64 framesRead = 0;
      const ma_result result =
          ma_decoder_read_pcm_frames(input.get(), buffer.data(), miniaudioChunkFrames, &framesRead);
      if (result != MA_SUCCESS && result != MA_AT_END) {
        return Err(
            "Failed to decode frames from '" + input.filePath() +
            "' with miniaudio: " + parseMiniAudioError(result));
      }

      if (framesRead == 0) {
        break;
      }

      auto writeResult = output.write(input.filePath(), buffer.data(), framesRead);
      if (writeResult.is_err()) {
        return writeResult;
      }

      if (result == MA_AT_END) {
        break;
      }
    }
  }

  return Ok(outputPath);
}

#if !RN_AUDIO_API_FFMPEG_DISABLED
const char *getMuxerNameForOutputPath(const std::string &outputPath) {
  const std::string extension = lowercaseExtension(outputPath);

  if (extension == "m4a" || extension == "mp4") {
    return "mp4";
  }

  return nullptr;
}

std::string parseFFmpegError(int errorCode) {
  std::array<char, AV_ERROR_MAX_STRING_SIZE> errorBuffer{};

  if (av_strerror(errorCode, errorBuffer.data(), sizeof(errorBuffer)) < 0) {
    return "Unknown FFmpeg error: " + std::to_string(errorCode);
  }

  return {errorBuffer.data()};
}

int findAudioStreamIndex(AVFormatContext *formatContext) {
  for (unsigned int i = 0; i < formatContext->nb_streams; ++i) {
    if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

bool hasUsableAudioParameters(const AVStream *stream) {
  const auto *codecParameters = stream->codecpar;
  return codecParameters->codec_id != AV_CODEC_ID_NONE && codecParameters->sample_rate > 0 &&
      codecParameters->ch_layout.nb_channels > 0;
}

using InputOpenResult = Result<InputFormatContext, std::string>;

InputOpenResult openInput(const std::string &filePath) {
  AVFormatContext *rawContext = nullptr;
  AVDictionaryGuard openOptions;
  av_dict_set(openOptions.ptr(), "protocol_whitelist", "file", 0);

  int result = avformat_open_input(&rawContext, filePath.c_str(), nullptr, openOptions.ptr());

  if (result < 0) {
    return Err("Failed to open input file '" + filePath + "': " + parseFFmpegError(result));
  }

  std::unique_ptr<AVFormatContext, void (*)(AVFormatContext *)> guard(
      rawContext, [](AVFormatContext *context) {
        if (context != nullptr) {
          avformat_close_input(&context);
        }
      });

  int audioStreamIndex = findAudioStreamIndex(rawContext);
  if (audioStreamIndex < 0) {
    return Err("Input file '" + filePath + "' does not contain an audio stream.");
  }

  if (!hasUsableAudioParameters(rawContext->streams[audioStreamIndex])) {
    AVDictionaryGuard streamInfoOptions;
    av_dict_set(streamInfoOptions.ptr(), "protocol_whitelist", "file", 0);

    result = avformat_find_stream_info(rawContext, streamInfoOptions.ptr());
    if (result < 0) {
      return Err("Failed to read stream info from '" + filePath + "': " + parseFFmpegError(result));
    }

    audioStreamIndex = findAudioStreamIndex(rawContext);
    if (audioStreamIndex < 0) {
      return Err("Input file '" + filePath + "' does not contain an audio stream.");
    }
  }

  if (!hasUsableAudioParameters(rawContext->streams[audioStreamIndex])) {
    return Err("Input file '" + filePath + "' is missing required audio parameters.");
  }

  return Ok(InputFormatContext(filePath, guard.release(), audioStreamIndex));
}

AudioFileConcatResult validateCompatibleInput(
    const InputFormatContext &input,
    const InputFormatContext &reference) {
  const auto *candidate = input.audioStream()->codecpar;
  const auto *base = reference.audioStream()->codecpar;

  if (std::strcmp(input.context()->iformat->name, reference.context()->iformat->name) != 0) {
    return Err(
        "Input file '" + input.filePath() +
        "' uses a different container format than the first input.");
  }

  if (candidate->codec_type != base->codec_type || candidate->codec_id != base->codec_id) {
    return Err("Input file '" + input.filePath() + "' uses a different audio codec.");
  }

  if (candidate->sample_rate != base->sample_rate) {
    return Err("Input file '" + input.filePath() + "' uses a different sample rate.");
  }

  if (av_channel_layout_compare(&candidate->ch_layout, &base->ch_layout) != 0) {
    return Err("Input file '" + input.filePath() + "' uses a different channel layout.");
  }

  if (candidate->format != base->format ||
      candidate->bits_per_coded_sample != base->bits_per_coded_sample ||
      candidate->bits_per_raw_sample != base->bits_per_raw_sample ||
      candidate->profile != base->profile || candidate->level != base->level ||
      candidate->block_align != base->block_align || candidate->frame_size != base->frame_size) {
    return Err("Input file '" + input.filePath() + "' has incompatible audio parameters.");
  }

  if (candidate->extradata_size != base->extradata_size) {
    return Err("Input file '" + input.filePath() + "' has incompatible codec extradata.");
  }

  if (candidate->extradata_size > 0 &&
      std::memcmp(candidate->extradata, base->extradata, candidate->extradata_size) != 0) {
    return Err("Input file '" + input.filePath() + "' has incompatible codec extradata.");
  }

  return Ok(input.filePath());
}

AudioFileConcatResult openAndValidateInputs(
    const std::vector<std::string> &inputPaths,
    std::vector<InputFormatContext> &inputs) {
  inputs.reserve(inputPaths.size());

  for (const auto &inputPath : inputPaths) {
    auto inputResult = openInput(inputPath);
    if (inputResult.is_err()) {
      return Err(inputResult.unwrap_err());
    }

    inputs.emplace_back(std::move(inputResult).unwrap());

    if (inputs.size() > 1) {
      auto validationResult = validateCompatibleInput(inputs.back(), inputs.front());
      if (validationResult.is_err()) {
        return validationResult;
      }
    }
  }

  return Ok(std::string());
}

AudioFileConcatResult createOutput(
    const std::string &outputPath,
    const InputFormatContext &firstInput,
    std::unique_ptr<OutputFormatContext> &output,
    AVStream **outputStream) {
  if (firstInput.context()->iformat->extensions != nullptr &&
      av_match_ext(outputPath.c_str(), firstInput.context()->iformat->extensions) == 0) {
    return Err("concatAudioFiles output path must use an extension compatible with the inputs.");
  }

  const char *muxerName = getMuxerNameForOutputPath(outputPath);
  const AVOutputFormat *outputFormat = av_guess_format(muxerName, outputPath.c_str(), nullptr);
  if (outputFormat == nullptr) {
    return Err("Failed to determine output format for '" + outputPath + "'.");
  }

  AVFormatContext *rawOutputContext = nullptr;
  int result = avformat_alloc_output_context2(
      &rawOutputContext, outputFormat, muxerName, outputPath.c_str());

  if (result < 0 || rawOutputContext == nullptr) {
    return Err("Failed to allocate output context: " + parseFFmpegError(result));
  }

  output = std::make_unique<OutputFormatContext>(rawOutputContext);
  *outputStream = avformat_new_stream(rawOutputContext, nullptr);

  if (*outputStream == nullptr) {
    return Err("Failed to create output audio stream.");
  }

  auto *inputStream = firstInput.audioStream();
  result = avcodec_parameters_copy((*outputStream)->codecpar, inputStream->codecpar);
  if (result < 0) {
    return Err("Failed to copy audio stream parameters: " + parseFFmpegError(result));
  }

  (*outputStream)->codecpar->codec_tag = 0;
  (*outputStream)->time_base = AVRational{.num = 1, .den = inputStream->codecpar->sample_rate};

  if (!(rawOutputContext->oformat->flags & AVFMT_NOFILE)) {
    result = avio_open(&rawOutputContext->pb, outputPath.c_str(), AVIO_FLAG_WRITE);
    if (result < 0) {
      return Err("Failed to open output file '" + outputPath + "': " + parseFFmpegError(result));
    }
  }

  result = avformat_write_header(rawOutputContext, nullptr);
  if (result < 0) {
    return Err("Failed to write output header: " + parseFFmpegError(result));
  }

  return Ok(outputPath);
}

int64_t firstValidTimestamp(const AVPacket *packet) {
  if (packet->dts != AV_NOPTS_VALUE) {
    return packet->dts;
  }
  if (packet->pts != AV_NOPTS_VALUE) {
    return packet->pts;
  }
  return 0;
}

int64_t packetEndTimestamp(const AVPacket *packet, AVRational outputTimeBase) {
  int64_t endTimestamp = AV_NOPTS_VALUE;
  if (packet->pts != AV_NOPTS_VALUE) {
    endTimestamp = packet->pts;
  }
  if (packet->dts != AV_NOPTS_VALUE &&
      (endTimestamp == AV_NOPTS_VALUE || packet->dts > endTimestamp)) {
    endTimestamp = packet->dts;
  }
  if (endTimestamp == AV_NOPTS_VALUE) {
    return AV_NOPTS_VALUE;
  }
  if (packet->duration > 0) {
    endTimestamp += packet->duration;
  }
  return av_rescale_q(endTimestamp, outputTimeBase, outputTimeBase);
}

AudioFileConcatResult appendInputPackets(
    InputFormatContext &input,
    AVFormatContext *outputContext,
    AVStream *outputStream,
    int64_t &timestampOffset) {
  auto *inputStream = input.audioStream();
  const AVRational inputTimeBase = inputStream->time_base;
  const AVRational outputTimeBase = outputStream->time_base;
  AVPacket *packet = av_packet_alloc();

  if (packet == nullptr) {
    return Err("Failed to allocate FFmpeg packet.");
  }

  std::unique_ptr<AVPacket, void (*)(AVPacket *)> packetGuard(
      packet, [](AVPacket *value) { av_packet_free(&value); });

  int64_t baseTimestamp = AV_NOPTS_VALUE;
  int64_t segmentEndTimestamp = timestampOffset;

  while (true) {
    int result = av_read_frame(input.context(), packet);
    if (result == AVERROR_EOF) {
      break;
    }
    if (result < 0) {
      return Err(
          "Failed to read packet from '" + input.filePath() + "': " + parseFFmpegError(result));
    }

    if (packet->stream_index != input.audioStreamIndex()) {
      av_packet_unref(packet);
      continue;
    }

    if (baseTimestamp == AV_NOPTS_VALUE) {
      baseTimestamp = firstValidTimestamp(packet);
    }

    if (packet->pts != AV_NOPTS_VALUE) {
      packet->pts = av_rescale_q(packet->pts - baseTimestamp, inputTimeBase, outputTimeBase) +
          timestampOffset;
    }
    if (packet->dts != AV_NOPTS_VALUE) {
      packet->dts = av_rescale_q(packet->dts - baseTimestamp, inputTimeBase, outputTimeBase) +
          timestampOffset;
    }
    if (packet->duration > 0) {
      packet->duration = av_rescale_q(packet->duration, inputTimeBase, outputTimeBase);
    }
    packet->pos = -1;
    packet->stream_index = outputStream->index;

    const int64_t packetEnd = packetEndTimestamp(packet, outputTimeBase);
    if (packetEnd != AV_NOPTS_VALUE && packetEnd > segmentEndTimestamp) {
      segmentEndTimestamp = packetEnd;
    }

    result = av_interleaved_write_frame(outputContext, packet);
    if (result < 0) {
      av_packet_unref(packet);
      return Err(
          "Failed to write packet from '" + input.filePath() + "': " + parseFFmpegError(result));
    }
  }

  if (inputStream->duration != AV_NOPTS_VALUE && inputStream->duration > 0) {
    const int64_t streamEnd =
        timestampOffset + av_rescale_q(inputStream->duration, inputTimeBase, outputTimeBase);
    segmentEndTimestamp = std::max(segmentEndTimestamp, streamEnd);
  }

  timestampOffset = segmentEndTimestamp;
  return Ok(input.filePath());
}

AudioFileConcatResult concatAudioFilesWithFFmpeg(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  std::vector<InputFormatContext> inputs;
  auto inputValidationResult = openAndValidateInputs(inputPaths, inputs);
  if (inputValidationResult.is_err()) {
    return inputValidationResult;
  }

  std::unique_ptr<OutputFormatContext> output;
  AVStream *outputStream = nullptr;
  auto outputResult = createOutput(outputPath, inputs.front(), output, &outputStream);
  if (outputResult.is_err()) {
    return outputResult;
  }

  int64_t timestampOffset = 0;
  for (auto &input : inputs) {
    auto appendResult = appendInputPackets(input, output->get(), outputStream, timestampOffset);
    if (appendResult.is_err()) {
      return appendResult;
    }
  }

  int result = av_write_trailer(output->get());
  if (result < 0) {
    return Err("Failed to write output trailer: " + parseFFmpegError(result));
  }

  return Ok(outputPath);
}
#endif // RN_AUDIO_API_FFMPEG_DISABLED

} // namespace

std::string normalizeFilePath(const std::string &path) {
  if (path.starts_with(fileUrlPrefix)) {
    return percentDecode(path.substr(std::strlen(fileUrlPrefix)));
  }

  return percentDecode(path);
}

AudioFileConcatResult concatAudioFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  std::vector<std::string> normalizedInputPaths;
  normalizedInputPaths.reserve(inputPaths.size());
  for (const auto &inputPath : inputPaths) {
    normalizedInputPaths.push_back(normalizeFilePath(inputPath));
  }

  const std::string normalizedOutputPath = normalizeFilePath(outputPath);

  auto pathValidationResult = validatePaths(normalizedInputPaths, normalizedOutputPath);
  if (pathValidationResult.is_err()) {
    return pathValidationResult;
  }

  if (isMiniaudioOutputPath(normalizedOutputPath)) {
    return concatAudioFilesWithMiniAudio(normalizedInputPaths, normalizedOutputPath)
        .map([&outputPath](const std::string &) { return outputPath; });
  }

  if (!isFFmpegOutputPath(normalizedOutputPath)) {
    return Err(
        "concatAudioFiles supports WAV output with miniaudio and M4A/MP4 output with FFmpeg.");
  }

#if RN_AUDIO_API_FFMPEG_DISABLED
  return Err("FFmpeg is disabled, cannot concatenate M4A/MP4 audio files.");
#else
  return concatAudioFilesWithFFmpeg(normalizedInputPaths, normalizedOutputPath)
      .map([&outputPath](const std::string &) { return outputPath; });
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

} // namespace audioapi
