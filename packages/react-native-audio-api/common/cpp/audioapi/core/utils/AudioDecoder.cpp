#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/libs/decoding/IncrementalAudioDecoder.h>
#include <audioapi/libs/miniaudio/MiniAudioDecoding.h>
#include <audioapi/utils/AudioArray.hpp>

#if !RN_AUDIO_API_FFMPEG_DISABLED
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#endif // RN_AUDIO_API_FFMPEG_DISABLED

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

// Drains an incremental decoder into an AudioBuffer. Total frame count is not
// known up front for some formats (e.g. Vorbis), so we read in fixed-size
// chunks and grow the interleaved accumulator until the decoder reports EOF.
AudioBufferResult decodeAll(decoding::IncrementalAudioDecoder &decoder) {
  const int channels = std::max(1, decoder.outputChannels());
  const auto outputSampleRate = static_cast<float>(decoder.outputSampleRate());

  std::vector<float> interleaved;
  std::vector<float> chunk(
      decoding::IncrementalAudioDecoder::CHUNK_SIZE * static_cast<size_t>(channels));

  while (true) {
    const size_t framesRead =
        decoder.readPcmFrames(chunk.data(), decoding::IncrementalAudioDecoder::CHUNK_SIZE);
    if (framesRead == 0) {
      break;
    }
    interleaved.insert(
        interleaved.end(),
        chunk.begin(),
        chunk.begin() + static_cast<std::ptrdiff_t>(framesRead * static_cast<size_t>(channels)));
  }

  if (interleaved.empty()) {
    return Err("Failed to decode any frames");
  }

  const size_t outputFrames = interleaved.size() / static_cast<size_t>(channels);
  auto audioBuffer = std::make_shared<AudioBuffer>(outputFrames, channels, outputSampleRate);
  audioBuffer->deinterleaveFrom(interleaved.data(), outputFrames);
  return Ok(std::move(audioBuffer));
}

bool needsFFmpeg(AudioFormat format) {
  return format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC;
}

bool needsFFmpegByPath(const std::string &path) {
  return AudioDecoder::pathHasExtension(path, {".mp4", ".m4a", ".aac"});
}

AudioBufferResult AudioDecoder::decodeWithFilePath(const std::string &path, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);

  if (needsFFmpegByPath(path)) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    ffmpegdecoder::FFmpegDecoder decoder;
    if (!decoder.openFile(sr, path)) {
      return Err("Failed to open file with FFmpeg decoder");
    }
    auto result = decodeAll(decoder);
    decoder.close();
    return result;
#else
    return Err("FFmpeg is disabled, cannot decode with file path");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }

  miniaudio_decoder::MiniAudioDecoder decoder;
  if (!decoder.openFile(sr, path)) {
    return Err("Failed to open file with miniaudio decoder");
  }
  auto result = decodeAll(decoder);
  decoder.close();
  return result;
}

AudioBufferResult
AudioDecoder::decodeWithMemoryBlock(const void *data, size_t size, float sampleRate) {
  const int sr = static_cast<int>(sampleRate);
  const AudioFormat format = AudioDecoder::detectAudioFormat(data, size);

  if (needsFFmpeg(format)) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
    ffmpegdecoder::FFmpegDecoder decoder;
    if (!decoder.openMemory(sr, data, size)) {
      return Err("Failed to open memory block with FFmpeg decoder");
    }
    auto result = decodeAll(decoder);
    decoder.close();
    return result;
#else
    return Err("FFmpeg is disabled, cannot decode memory block");
#endif // RN_AUDIO_API_FFMPEG_DISABLED
  }

  miniaudio_decoder::MiniAudioDecoder decoder;
  if (!decoder.openMemory(sr, data, size)) {
    return Err("Failed to open memory block with miniaudio decoder");
  }
  auto result = decodeAll(decoder);
  decoder.close();
  return result;
}

AudioBufferResult AudioDecoder::decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved) {
  auto decodedData = base64_decode(data, false);
  const auto *uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t numFramesDecoded = decodedData.size() / (inputChannelCount * sizeof(int16_t));

  auto audioBuffer =
      std::make_shared<AudioBuffer>(numFramesDecoded, inputChannelCount, inputSampleRate);

  for (int ch = 0; ch < inputChannelCount; ++ch) {
    auto channelData = audioBuffer->getChannel(ch)->span();

    for (size_t i = 0; i < numFramesDecoded; ++i) {
      size_t offset;
      if (interleaved) {
        // Ch1, Ch2, Ch1, Ch2, ...
        offset = (i * inputChannelCount + ch) * sizeof(int16_t);
      } else {
        // Ch1, Ch1, Ch1, ..., Ch2, Ch2, Ch2, ...
        offset = (ch * numFramesDecoded + i) * sizeof(int16_t);
      }

      channelData[i] = uint8ToFloat(uint8Data[offset], uint8Data[offset + 1]);
    }
  }

  return Ok(std::move(audioBuffer));
}

} // namespace audioapi
