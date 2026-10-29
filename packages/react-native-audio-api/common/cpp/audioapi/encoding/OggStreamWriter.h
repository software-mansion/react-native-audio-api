#pragma once

#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#if !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED

#include <audioapi/external/include/ogg/ogg.h>

namespace audioapi::ogg_opus {

/// One Ogg logical stream written straight to a file through libogg, as the encoder and the
/// remux both need: packets go in, and the pages they complete go out to the file.
class OggStreamWriter {
 public:
  OggStreamWriter() = default;
  ~OggStreamWriter();
  DELETE_COPY_AND_MOVE(OggStreamWriter);

  /// Creates or truncates @p path and opens a stream under a random serial number.
  Result<NoneType, std::string> open(const std::string &path);
  /// Releases the stream and the file. A page libogg has not completed is lost with them, so
  /// writePages(true) first when it matters.
  void close();
  [[nodiscard]] bool isOpen() const;

  /// Queues the next packet. libogg copies the bytes in, so @p data may go once this returns.
  Result<NoneType, std::string> submit(
      const unsigned char *data,
      size_t size,
      int64_t granulePosition,
      bool beginningOfStream,
      bool endOfStream);
  /// Writes the pages libogg has completed; with @p forceFlush also the partial one.
  Result<NoneType, std::string> writePages(bool forceFlush);
  /// Hands what the C runtime still buffers to the OS.
  Result<NoneType, std::string> flush();

  /// Bytes written so far, readable from any thread.
  [[nodiscard]] size_t bytesWritten() const;

 private:
  Result<NoneType, std::string> writeBytes(const unsigned char *data, size_t count);

  std::FILE *file_ = nullptr;
  ogg_stream_state stream_{};
  bool streamReady_ = false;
  int64_t packetNumber_ = 0;
  std::atomic<size_t> bytesWritten_{0};
  std::string path_;
};

} // namespace audioapi::ogg_opus

#endif // !RN_AUDIO_API_STATIC_EXTERNAL_LIBS_DISABLED
