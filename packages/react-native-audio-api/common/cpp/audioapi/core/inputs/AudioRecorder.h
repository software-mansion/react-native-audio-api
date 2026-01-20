#pragma once

#include <audioapi/utils/Result.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

namespace audioapi {

class AudioBus;
class AudioFileWriter;
class CircularAudioArray;
class RecorderAdapterNode;
class AudioFileProperties;
class AudioRecorderCallback;
class AudioEventHandlerRegistry;

class AudioRecorder {
 public:
  enum class RecorderState { Idle = 0, Recording, Paused };
  explicit AudioRecorder(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry)
      : audioEventHandlerRegistry_(audioEventHandlerRegistry) {}
  virtual ~AudioRecorder() = default;

  virtual Result<std::string, std::string> start(const std::string &fileNameOverride) = 0;
  virtual Result<std::tuple<std::string, double, double>, std::string> stop() = 0;

  virtual Result<std::string, std::string> enableFileOutput(
      std::shared_ptr<AudioFileProperties> properties) = 0;
  virtual void disableFileOutput() = 0;

  virtual void pause() = 0;
  virtual void resume() = 0;

  virtual void connect(const std::shared_ptr<RecorderAdapterNode> &node) = 0;
  virtual void disconnect() = 0;

  virtual Result<NoneType, std::string> setOnAudioReadyCallback(
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId) = 0;
  virtual void clearOnAudioReadyCallback() = 0;

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();

  virtual double getCurrentDuration() const;

  bool usesCallback() const;
  bool usesFileOutput() const;
  bool isConnected() const;

  virtual bool isRecording() const = 0;
  virtual bool isPaused() const = 0;
  virtual bool isIdle() const = 0;

 protected:
  std::atomic<RecorderState> state_{RecorderState::Idle};

  std::atomic<bool> isConnected_{false};
  std::atomic<bool> fileOutputEnabled_{false};
  std::atomic<bool> callbackOutputEnabled_{false};

  std::mutex callbackMutex_;
  std::mutex fileWriterMutex_;
  std::mutex errorCallbackMutex_;
  mutable std::mutex adapterNodeMutex_;

  std::atomic<uint64_t> errorCallbackId_{0};

  std::string filePath_{""};
  std::shared_ptr<AudioFileWriter> fileWriter_ = nullptr;
  std::shared_ptr<RecorderAdapterNode> adapterNode_ = nullptr;
  std::shared_ptr<AudioRecorderCallback> dataCallback_ = nullptr;
  std::shared_ptr<AudioEventHandlerRegistry> audioEventHandlerRegistry_;
};

} // namespace audioapi
