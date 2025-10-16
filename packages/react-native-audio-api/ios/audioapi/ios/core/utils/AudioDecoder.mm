#define MINIAUDIO_IMPLEMENTATION
#import <audioapi/libs/miniaudio/miniaudio.h>

#include <audioapi/libs/miniaudio/decoders/libopus/miniaudio_libopus.h>
#include <audioapi/libs/miniaudio/decoders/libvorbis/miniaudio_libvorbis.h>

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/utils/AudioDecoder.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/libs/audio-stretch/stretch.h>
#include <audioapi/libs/base64/base64.h>
#include <audioapi/libs/ffmpeg/FFmpegDecoding.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

// Decoding audio in fixed-size chunks because total frame count can't be
// determined in advance. Note: ma_decoder_get_length_in_pcm_frames() always
// returns 0 for Vorbis decoders.
std::vector<float> AudioDecoder::readAllPcmFrames(ma_decoder &decoder, int outputChannels)
{
  std::vector<float> buffer;
  std::vector<float> temp(CHUNK_SIZE * outputChannels);
  ma_uint64 outFramesRead = 0;

  while (true) {
    ma_uint64 tempFramesDecoded = 0;
    ma_decoder_read_pcm_frames(&decoder, temp.data(), CHUNK_SIZE, &tempFramesDecoded);
    if (tempFramesDecoded == 0) {
      break;
    }

    buffer.insert(buffer.end(), temp.data(), temp.data() + tempFramesDecoded * outputChannels);
    outFramesRead += tempFramesDecoded;
  }

  if (outFramesRead == 0) {
    NSLog(@"Failed to decode");
  }
  return buffer;
}

std::shared_ptr<AudioBuffer> AudioDecoder::makeAudioBufferFromFloatBuffer(
    const std::vector<float> &buffer,
    float outputSampleRate,
    int outputChannels)
{
  if (buffer.empty()) {
    return nullptr;
  }

  auto outputFrames = buffer.size() / outputChannels;
  auto audioBus = std::make_shared<AudioBus>(outputFrames, outputChannels, outputSampleRate);

  for (int ch = 0; ch < outputChannels; ++ch) {
    auto channelData = audioBus->getChannel(ch)->getData();
    for (int i = 0; i < outputFrames; ++i) {
      channelData[i] = buffer[i * outputChannels + ch];
    }
  }
  return std::make_shared<AudioBuffer>(audioBus);
}

std::shared_ptr<AudioBuffer> AudioDecoder::decodeWithFilePath(const std::string &path, float sampleRate)
{
  if (AudioDecoder::pathHasExtension(path, {".mp4", ".m4a", ".aac"})) {
    auto buffer = ffmpegdecoder::decodeWithFilePath(path, static_cast<int>(sampleRate));
    if (buffer == nullptr) {
      NSLog(@"Failed to decode with FFmpeg: %s", path.c_str());
      return nullptr;
    }
    return buffer;
  }
  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, static_cast<int>(sampleRate));
  ma_decoding_backend_vtable *customBackends[] = {ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

  if (ma_decoder_init_file(path.c_str(), &config, &decoder) != MA_SUCCESS) {
    NSLog(@"Failed to initialize decoder for file: %s", path.c_str());
    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  auto outputSampleRate = static_cast<float>(decoder.outputSampleRate);
  auto outputChannels = static_cast<int>(decoder.outputChannels);

  std::vector<float> buffer = readAllPcmFrames(decoder, outputChannels);
  ma_decoder_uninit(&decoder);
  return makeAudioBufferFromFloatBuffer(buffer, outputSampleRate, outputChannels);
}

std::shared_ptr<AudioBuffer> AudioDecoder::decodeWithMemoryBlock(const void *data, size_t size, float sampleRate)
{
  const AudioFormat format = AudioDecoder::detectAudioFormat(data, size);
  if (format == AudioFormat::MP4 || format == AudioFormat::M4A || format == AudioFormat::AAC) {
    auto buffer = ffmpegdecoder::decodeWithMemoryBlock(data, size, static_cast<int>(sampleRate));
    if (buffer == nullptr) {
      NSLog(@"Failed to decode with FFmpeg");
      return nullptr;
    }
    return buffer;
  }
  ma_decoder decoder;
  ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, static_cast<int>(sampleRate));

  ma_decoding_backend_vtable *customBackends[] = {ma_decoding_backend_libvorbis, ma_decoding_backend_libopus};

  config.ppCustomBackendVTables = customBackends;
  config.customBackendCount = sizeof(customBackends) / sizeof(customBackends[0]);

  if (ma_decoder_init_memory(data, size, &config, &decoder) != MA_SUCCESS) {
    NSLog(@"Failed to initialize decoder for memory block");
    ma_decoder_uninit(&decoder);
    return nullptr;
  }

  auto outputSampleRate = static_cast<float>(decoder.outputSampleRate);
  auto outputChannels = static_cast<int>(decoder.outputChannels);

  std::vector<float> buffer = readAllPcmFrames(decoder, outputChannels);
  ma_decoder_uninit(&decoder);
  return makeAudioBufferFromFloatBuffer(buffer, outputSampleRate, outputChannels);
}

std::shared_ptr<AudioBuffer> AudioDecoder::decodeWithPCMInBase64(
    const std::string &data,
    float inputSampleRate,
    int inputChannelCount,
    bool interleaved)
{
  auto decodedData = base64_decode(data, false);
  const auto uint8Data = reinterpret_cast<uint8_t *>(decodedData.data());
  size_t numFramesDecoded = decodedData.size() / (inputChannelCount * sizeof(int16_t));

  auto audioBus = std::make_shared<AudioBus>(numFramesDecoded, inputChannelCount, inputSampleRate);

  for (int ch = 0; ch < inputChannelCount; ++ch) {
    auto channelData = audioBus->getChannel(ch)->getData();

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
  return std::make_shared<AudioBuffer>(audioBus);
}

} // namespace audioapi
