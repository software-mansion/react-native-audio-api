#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioBufferPool.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

namespace audioapi {

class AudioFileProperties;
class IAudioEventHandlerRegistry;

using OpenFileResult = Result<std::string, std::string>;
using CloseFileResult = Result<std::tuple<double, double>, std::string>;

/// A filled pool buffer on its way to the worker. The default-constructed value (no buffer)
/// is the offloader's shutdown message, which is what operator== exists for.
struct PendingFileWrite {
  AudioBufferLease buffer;
  int numFrames = 0;

  bool operator==(const PendingFileWrite &) const = default;
};

struct PlatformFileBackend {
  std::function<Result<EncoderOutputSpec, std::string>(AudioFileProperties::Format)>
      resolveOutputSpec;
  std::function<Result<std::string, std::string>(
      const std::shared_ptr<AudioFileProperties> &,
      const std::string &fileName)>
      resolvePath;
  std::function<std::unique_ptr<AudioEncoder>(const std::shared_ptr<AudioFileProperties> &)>
      createEncoder;
  /// Points an open encoder at a new input format while the file stays the same.
  std::function<OpenEncoderResult(AudioEncoder &, const StreamFormat &, size_t maxFramesPerBuffer)>
      reprepareEncoderInput;
};

/// The iOS and Android implementations of the steps above.
PlatformFileBackend createOsFileBackend();

class AudioFileWriter final {
 public:
  using OnFileOpenedCallback = std::function<void(const std::string &)>;

  AudioFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      OnFileOpenedCallback onFileOpened = {},
      PlatformFileBackend backend = createOsFileBackend());
  ~AudioFileWriter();
  DELETE_COPY_AND_MOVE(AudioFileWriter);

  /// JS thread. @p maxFramesPerBuffer bounds a single writeAudioData() call.
  /// Returns the opened file's path.
  OpenFileResult
  openFile(float streamSampleRate, int32_t streamChannelCount, int32_t maxFramesPerBuffer);

  /// JS thread. Returns {sizeMB, durationSeconds} summed over every file of the session.
  CloseFileResult closeFile();

  /// ios only because android handles input format changes automatically. Returns the file path on success.
  OpenFileResult reprepareStreamFormat(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer);

  /// Audio thread. @p channels holds one pointer per stream channel, each to numFrames float32
  /// samples, valid only for the call. Never blocks; drops the buffer when no pool slot is free.
  void writeAudioData(const float *const *channels, int numFrames);

  [[nodiscard]] std::string getFilePath() const;
  /// Across every file of the session.
  [[nodiscard]] double getCurrentDuration() const;
  [[nodiscard]] size_t getFileSizeBytes() const;

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();

 private:
  static constexpr auto FILE_WRITER_SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto FILE_WRITER_SPSC_WAIT_STRATEGY = channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr size_t FILE_WRITER_POOL_SIZE = 32;
  // SPSC rings hold at most (capacity - 1) elements.
  static constexpr auto FILE_WRITER_CHANNEL_CAPACITY = FILE_WRITER_POOL_SIZE + 1;
  static_assert(
      FILE_WRITER_POOL_SIZE <= FILE_WRITER_CHANNEL_CAPACITY - 1,
      "Channel must hold every in-flight slot so send() never blocks/overwrites");
  /// Checking the file size costs a stat(), so only do it every Nth encoded buffer.
  static constexpr int FILE_SIZE_CHECK_WRITE_INTERVAL = 10;

  using Offloader = task_offloader::TaskOffloader<
      PendingFileWrite,
      FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      FILE_WRITER_SPSC_WAIT_STRATEGY>;

  [[nodiscard]] bool isFileOpen() const;
  [[nodiscard]] bool rotatesFiles() const;

  /// JS thread, with no worker running: also sizes the buffer pool and starts the worker.
  OpenFileResult
  startNextFile(float streamSampleRate, int32_t streamChannelCount, int32_t maxFramesPerBuffer);
  /// JS thread. Joins the worker, then folds the file into the session totals.
  CloseEncoderResult finishCurrentFile();

  /// @p fileNumber is 1-based. A rotated session numbers every file; one that is not has a
  /// single file under the plain stem.
  [[nodiscard]] std::string fileStem(size_t fileNumber) const;
  [[nodiscard]] Result<std::string, std::string> resolveNextFilePath(
      const std::string &stem,
      const std::string &extension) const;
  /// The caller must hold fileMutex_.
  OpenFileResult openEncoderForNextFile();
  /// The caller must hold fileMutex_.
  OpenFileResult reprepareEncoderInput();
  /// The caller must hold fileMutex_.
  CloseEncoderResult retireEncoder();
  /// The caller must hold fileMutex_.
  void foldFinishedFile(const std::tuple<double, double> &finished);
  void rollbackFailedOpen();

  /// Worker thread, once per encoded buffer. Swaps the encoder underneath the running worker.
  void rotateOnceFileOutgrowsCap();
  void announceFileOpened(const std::string &path);
  void invokeOnErrorCallback(const std::string &message);

  bool initializePreallocatedInputPool();
  void cleanupPreallocatedInputPool();
  void createOffloader();
  void runWriterTask(PendingFileWrite pending);

  std::shared_ptr<AudioFileProperties> fileProperties_;
  OnFileOpenedCallback onFileOpened_;
  /// Declared before offloader_, so the worker thread that calls into it is joined first.
  PlatformFileBackend backend_;
  EventCaller<AudioEvent::RECORDER_ERROR> errorEvent_;

  std::atomic<bool> isFileOpen_{false};
  std::atomic<size_t> framesWritten_{0};

  float streamSampleRate_{0.0F};
  int32_t streamChannelCount_{0};
  int32_t maxFramesPerBuffer_{0};

  /// Guards the members below, which a rotation advances on the worker thread while the JS
  /// thread reads them. Never taken on the audio thread.
  mutable std::mutex fileMutex_;
  std::string filePath_;
  std::unique_ptr<AudioEncoder> encoder_;
  std::string sessionStem_;
  size_t openedFileCount_{0};
  int writesSinceLastSizeCheck_{0};
  double finishedFilesSizeMB_{0.0};
  double finishedFilesDurationSec_{0.0};
  /// What the current file held before its last input-format change; framesWritten_ counts
  /// in the current stream rate only. The encoder reports the whole file on close.
  double currentFileEarlierFormatsDurationSec_{0.0};

  /// Planar buffers of maxFramesPerBuffer_ x streamChannelCount_ that carry audio-thread
  /// callbacks to the worker.
  AudioBufferPool<FILE_WRITER_POOL_SIZE> inputBufferPool_;
  std::unique_ptr<Offloader> offloader_;
};

} // namespace audioapi
