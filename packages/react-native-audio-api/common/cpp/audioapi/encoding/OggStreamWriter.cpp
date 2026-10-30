#include <audioapi/encoding/OggStreamWriter.h>

#if !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED

#include <cstdio>
#include <random>
#include <string>

namespace audioapi::ogg_opus {

OggStreamWriter::~OggStreamWriter() {
  close();
}

Result<NoneType, std::string> OggStreamWriter::open(const std::string &path) {
  close();
  file_ = std::fopen(path.c_str(), "wb");
  if (file_ == nullptr) {
    return Err("Failed to open '" + path + "' for writing");
  }
  std::random_device randomDevice;
  if (ogg_stream_init(&stream_, static_cast<int>(randomDevice())) != 0) {
    close();
    return Err("Failed to initialize the Ogg stream of '" + path + "'");
  }
  streamReady_ = true;
  path_ = path;
  packetNumber_ = 0;
  bytesWritten_.store(0, std::memory_order_release);
  return Ok(None);
}

void OggStreamWriter::close() {
  if (streamReady_) {
    ogg_stream_clear(&stream_);
    streamReady_ = false;
  }
  if (file_ != nullptr) {
    std::fclose(file_);
    file_ = nullptr;
  }
}

bool OggStreamWriter::isOpen() const {
  return file_ != nullptr;
}

Result<NoneType, std::string> OggStreamWriter::submit(
    const unsigned char *data,
    size_t size,
    int64_t granulePosition,
    bool beginningOfStream,
    bool endOfStream) {
  ogg_packet packet{};
  // libogg copies the bytes in and never writes through the pointer.
  packet.packet =
      const_cast<unsigned char *>(data);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
  packet.bytes = static_cast<long>(size); // NOLINT(runtime/int)
  packet.b_o_s = beginningOfStream ? 1 : 0;
  packet.e_o_s = endOfStream ? 1 : 0;
  packet.granulepos = granulePosition;
  packet.packetno = packetNumber_++;
  if (ogg_stream_packetin(&stream_, &packet) != 0) {
    return Err("Failed to queue an Ogg packet of '" + path_ + "'");
  }
  return Ok(None);
}

Result<NoneType, std::string> OggStreamWriter::writePages(bool forceFlush) {
  ogg_page page{};
  while ((forceFlush ? ogg_stream_flush(&stream_, &page) : ogg_stream_pageout(&stream_, &page)) !=
         0) {
    auto headerResult = writeBytes(page.header, static_cast<size_t>(page.header_len));
    if (headerResult.is_err()) {
      return headerResult;
    }
    auto bodyResult = writeBytes(page.body, static_cast<size_t>(page.body_len));
    if (bodyResult.is_err()) {
      return bodyResult;
    }
  }
  return Ok(None);
}

Result<NoneType, std::string> OggStreamWriter::flush() {
  if (std::fflush(file_) != 0) {
    return Err("Failed to flush '" + path_ + "'");
  }
  return Ok(None);
}

size_t OggStreamWriter::bytesWritten() const {
  return bytesWritten_.load(std::memory_order_acquire);
}

Result<NoneType, std::string> OggStreamWriter::writeBytes(const unsigned char *data, size_t count) {
  if (count == 0) {
    return Ok(None);
  }
  if (std::fwrite(data, 1, count, file_) != count) {
    return Err("Short write to '" + path_ + "'");
  }
  bytesWritten_.fetch_add(count, std::memory_order_acq_rel);
  return Ok(None);
}

} // namespace audioapi::ogg_opus

#endif // !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
