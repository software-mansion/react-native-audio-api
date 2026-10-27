#include <audioapi/encoding/OggOpusRemux.h>
#include "audioapi/utils/Macros.h"

#if RN_AUDIO_API_BUNDLED_OGG_OPUS

#include <audioapi/encoding/OggStreamWriter.h>
#include <audioapi/external/include/ogg/ogg.h>
#include <audioapi/external/include/opus/opus.h>
#include <audioapi/utils/FileSystem.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace audioapi::ogg_opus {

namespace {

constexpr size_t READ_CHUNK_BYTES = 64 * 1024;
constexpr int OPUS_SAMPLE_RATE = 48000;
/// RFC 7845 §5.1: the fixed part of OpusHead; mapping families other than 0 append a table.
constexpr size_t OPUS_HEAD_MIN_BYTES = 19;
constexpr size_t OPUS_MAGIC_BYTES = 8;
constexpr size_t OPUS_HEAD_VERSION_OFFSET = 8;
constexpr size_t OPUS_HEAD_CHANNELS_OFFSET = 9;
constexpr unsigned int OPUS_HEAD_MAJOR_VERSION_SHIFT = 4;

using RemuxResult = Result<NoneType, std::string>;

struct ParsedOggOpusFile {
  std::vector<unsigned char> head;
  std::vector<unsigned char> tags;
  int channelCount = 0;
  std::vector<std::vector<unsigned char>> audioPackets;
  /// Decoder output frames of each audio packet, 48 kHz.
  std::vector<int> packetFrames;
  int64_t totalPacketFrames = 0;
  /// Where the file ends, as its last page's granule position says; at most totalPacketFrames.
  int64_t endGranulePosition = 0;
};

struct OggReader {
  std::FILE *file = nullptr;
  ogg_sync_state sync{};
  ogg_stream_state stream{};
  bool streamReady = false;
  int serialNumber = 0;

  OggReader() {
    ogg_sync_init(&sync);
  }
  ~OggReader() {
    if (streamReady) {
      ogg_stream_clear(&stream);
    }
    ogg_sync_clear(&sync);
    if (file != nullptr) {
      std::fclose(file);
    }
  }
  DELETE_COPY_AND_MOVE(OggReader);
};

bool hasMagic(const ogg_packet &packet, const char *magic) {
  return packet.bytes >= 0 && static_cast<size_t>(packet.bytes) >= OPUS_MAGIC_BYTES &&
      std::memcmp(packet.packet, magic, OPUS_MAGIC_BYTES) == 0;
}

RemuxResult takeHeaderPacket(
    ParsedOggOpusFile &parsed,
    const ogg_packet &packet,
    size_t packetIndex,
    const std::string &path) {
  if (packetIndex == 0) {
    if (!hasMagic(packet, "OpusHead") || static_cast<size_t>(packet.bytes) < OPUS_HEAD_MIN_BYTES ||
        (packet.packet[OPUS_HEAD_VERSION_OFFSET] >> OPUS_HEAD_MAJOR_VERSION_SHIFT) != 0) {
      return Err("Input file '" + path + "' is not an Ogg/Opus file.");
    }
    parsed.head.assign(packet.packet, packet.packet + packet.bytes);
    parsed.channelCount = packet.packet[OPUS_HEAD_CHANNELS_OFFSET];
    return Ok(None);
  }
  if (!hasMagic(packet, "OpusTags")) {
    return Err("Input file '" + path + "' is missing its OpusTags header.");
  }
  parsed.tags.assign(packet.packet, packet.packet + packet.bytes);
  return Ok(None);
}

RemuxResult
takeAudioPacket(ParsedOggOpusFile &parsed, const ogg_packet &packet, const std::string &path) {
  const int frames = opus_packet_get_nb_samples(
      packet.packet, static_cast<opus_int32>(packet.bytes), OPUS_SAMPLE_RATE);
  if (frames < 0) {
    return Err("Input file '" + path + "' holds a malformed Opus packet.");
  }
  parsed.audioPackets.emplace_back(packet.packet, packet.packet + packet.bytes);
  parsed.packetFrames.push_back(frames);
  parsed.totalPacketFrames += frames;
  return Ok(None);
}

/// Reads every packet of the single logical stream in @p path into memory.
Result<ParsedOggOpusFile, std::string> parseOggOpusFile(const std::string &path) {
  OggReader reader;
  reader.file = std::fopen(path.c_str(), "rb");
  if (reader.file == nullptr) {
    return Err("Failed to open input file '" + path + "'.");
  }

  ParsedOggOpusFile parsed;
  int64_t lastGranulePosition = -1;
  size_t packetIndex = 0;

  while (true) {
    char *buffer = ogg_sync_buffer(&reader.sync, READ_CHUNK_BYTES); // NOLINT(runtime/int)
    if (buffer == nullptr) {
      return Err("Failed to read input file '" + path + "'.");
    }
    const size_t bytesRead = std::fread(buffer, 1, READ_CHUNK_BYTES, reader.file);
    ogg_sync_wrote(&reader.sync, static_cast<long>(bytesRead)); // NOLINT(runtime/int)

    // -1 from libogg means bytes were skipped to find the next page after a hole or a damaged
    // page; the pages after it are intact.
    ogg_page page{};
    int pageStatus = 0;
    while ((pageStatus = ogg_sync_pageout(&reader.sync, &page)) != 0) {
      if (pageStatus < 0) {
        continue;
      }
      if (!reader.streamReady) {
        reader.serialNumber = ogg_page_serialno(&page);
        ogg_stream_init(&reader.stream, reader.serialNumber);
        reader.streamReady = true;
      } else if (ogg_page_serialno(&page) != reader.serialNumber) {
        return Err(
            "Input file '" + path +
            "' holds more than one Ogg stream, which concatAudioFiles does not support.");
      }
      if (ogg_stream_pagein(&reader.stream, &page) != 0) {
        continue;
      }
      if (ogg_page_granulepos(&page) >= 0) {
        lastGranulePosition = ogg_page_granulepos(&page);
      }

      ogg_packet packet{};
      int packetStatus = 0;
      while ((packetStatus = ogg_stream_packetout(&reader.stream, &packet)) != 0) {
        if (packetStatus < 0) {
          continue;
        }
        auto result = packetIndex < 2 ? takeHeaderPacket(parsed, packet, packetIndex, path)
                                      : takeAudioPacket(parsed, packet, path);
        if (result.is_err()) {
          return Err(std::move(result).unwrap_err());
        }
        ++packetIndex;
      }
    }

    if (bytesRead < READ_CHUNK_BYTES) {
      break;
    }
  }

  if (packetIndex < 2) {
    return Err("Input file '" + path + "' is not an Ogg/Opus file.");
  }
  // A file cut short has no final granule position; it then plays up to its last packet.
  parsed.endGranulePosition = lastGranulePosition < 0
      ? parsed.totalPacketFrames
      : std::min(lastGranulePosition, parsed.totalPacketFrames);
  return Ok(std::move(parsed));
}

RemuxResult writeJoinedStream(
    const std::vector<ParsedOggOpusFile> &inputs,
    const std::string &outputPath) {
  OggStreamWriter writer;
  auto opened = writer.open(outputPath);
  if (opened.is_err()) {
    return opened;
  }

  // Each header packet gets a page of its own (RFC 7845 §3).
  const ParsedOggOpusFile &first = inputs.front();
  for (const auto *header : {&first.head, &first.tags}) {
    auto submitted = writer.submit(header->data(), header->size(), 0, header == &first.head, false);
    if (submitted.is_err()) {
      return submitted;
    }
    auto written = writer.writePages(true);
    if (written.is_err()) {
      return written;
    }
  }

  int64_t framesBefore = 0;
  int64_t lastGranulePosition = 0;
  for (size_t inputIndex = 0; inputIndex < inputs.size(); ++inputIndex) {
    const ParsedOggOpusFile &input = inputs[inputIndex];
    const bool lastInput = inputIndex + 1 == inputs.size();
    int64_t framesInInput = 0;
    for (size_t packetIndex = 0; packetIndex < input.audioPackets.size(); ++packetIndex) {
      framesInInput += input.packetFrames[packetIndex];
      const bool lastPacket = lastInput && packetIndex + 1 == input.audioPackets.size();
      // Only the stream's end can trim; granule positions never decrease.
      const int64_t granulePosition = std::max(
          lastGranulePosition,
          framesBefore + (lastPacket ? input.endGranulePosition : framesInInput));
      lastGranulePosition = granulePosition;

      const auto &packet = input.audioPackets[packetIndex];
      auto submitted =
          writer.submit(packet.data(), packet.size(), granulePosition, false, lastPacket);
      if (submitted.is_err()) {
        return submitted;
      }
      auto written = writer.writePages(lastPacket);
      if (written.is_err()) {
        return written;
      }
    }
    framesBefore += input.totalPacketFrames;
  }

  return writer.flush();
}

} // namespace

Result<std::string, std::string> concatOggOpusFiles(
    const std::vector<std::string> &inputPaths,
    const std::string &outputPath) {
  std::vector<ParsedOggOpusFile> inputs;
  inputs.reserve(inputPaths.size());
  for (const auto &inputPath : inputPaths) {
    auto parsed = parseOggOpusFile(inputPath);
    if (parsed.is_err()) {
      return Err(std::move(parsed).unwrap_err());
    }
    if (!inputs.empty() && parsed.unwrap().channelCount != inputs.front().channelCount) {
      return Err("Input file '" + inputPath + "' uses a different channel count.");
    }
    inputs.push_back(std::move(parsed).unwrap());
  }

  auto written = writeJoinedStream(inputs, outputPath);
  if (written.is_err()) {
    // A truncated file at the requested path would pass for a finished one.
    file_system::removeFile(outputPath);
    return Err(std::move(written).unwrap_err());
  }
  return Ok(outputPath);
}

} // namespace audioapi::ogg_opus

#endif // RN_AUDIO_API_BUNDLED_OGG_OPUS
