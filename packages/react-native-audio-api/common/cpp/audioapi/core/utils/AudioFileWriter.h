#pragma once

#include <audioapi/utils/Result.hpp>
#include <atomic>
#include <memory>
#include <string>
#include <tuple>

namespace audioapi {

class AudioFileProperties;
class AudioEventHandlerRegistry;

typedef Result<std::string, std::string> OpenFileResult;
typedef Result<std::tuple<double, double>, std::string> CloseFileResult;

#ifdef ANDROID
typedef void *AudioDataType;
#else
#include <CoreAudio/CoreAudioTypes.h>
typedef const AudioBufferList *AudioDataType;
#endif

class AudioFileWriter {
 public:
  AudioFileWriter(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties);
  virtual ~AudioFileWriter() = default;

  virtual CloseFileResult closeFile() = 0;
  virtual OpenFileResult openFile() = 0;
  virtual std::string getFilePath() const = 0;

  virtual bool writeAudioData(AudioDataType data, int numFrames) = 0;

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
};

} // namespace audioapi
