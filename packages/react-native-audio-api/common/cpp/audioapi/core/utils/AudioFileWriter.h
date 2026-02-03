#pragma once

#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <tuple>

namespace audioapi {

class AudioFileProperties;
class AudioEventHandlerRegistry;

typedef Result<std::string, std::string> OpenFileResult;
typedef Result<std::tuple<double, double>, std::string> CloseFileResult;

class AudioFileWriter {
 public:
  AudioFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties);
  virtual ~AudioFileWriter() = default;

  virtual CloseFileResult closeFile() = 0;

  virtual std::string getFilePath() const = 0;
  virtual double getCurrentDuration() const = 0;

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();
  void invokeOnErrorCallback(const std::string &message);

 protected:
  bool isFileOpen();

  std::atomic<bool> isFileOpen_{false};
  std::atomic<size_t> framesWritten_{0};
  std::atomic<uint64_t> errorCallbackId_{0};

  std::shared_ptr<AudioFileProperties> fileProperties_;
  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;

  static constexpr auto FILE_WRITER_SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto FILE_WRITER_SPSC_WAIT_STRATEGY = channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr auto FILE_WRITER_CHANNEL_CAPACITY = 64;
};

} // namespace audioapi
