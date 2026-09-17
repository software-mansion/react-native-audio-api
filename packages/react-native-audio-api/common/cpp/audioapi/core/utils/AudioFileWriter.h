#pragma once

#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SlotFreeList.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

namespace audioapi {

class AudioFileProperties;
class IAudioEventHandlerRegistry;

using OpenFileResult = Result<std::string, std::string>;
using CloseFileResult = Result<std::tuple<double, double>, std::string>;

struct PendingFileWrite {
  size_t slot = std::numeric_limits<size_t>::max();
  int numFrames = 0;
};

/// The two steps only the platform can perform, so that a test can supply its own: the desktop
/// build has neither a recording directory to resolve into nor a system encoder to create.
struct PlatformFileBackend {
  std::function<Result<std::string, std::string>(
      const std::shared_ptr<AudioFileProperties> &,
      const std::string &fileName)>
      resolvePath;
  std::function<std::unique_ptr<AudioEncoder>(const std::shared_ptr<AudioFileProperties> &)>
      createEncoder;
};

/// The iOS and Android implementations of the two steps above.
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

  /// JS thread. Finishes the current file and continues the session in a new one opened for
  /// the new format. Returns that file's path.
  OpenFileResult reprepareStreamFormat(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer);

  /// Audio thread. @p interleavedFrames holds numFrames * channelCount interleaved float32
  /// samples, valid only for the call. Never blocks; drops the buffer when no pool slot is free.
  void writeAudioData(const float *interleavedFrames, int numFrames);

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

  using FreeList = slots::SlotFreeList<FILE_WRITER_POOL_SIZE>;
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

  /// @p fileNumber is 1-based. A rotated session numbers every file; one that is not keeps the
  /// plain stem for its first file and numbers the files reopened after a format change from 1.
  [[nodiscard]] std::string fileStem(size_t fileNumber) const;
  /// The caller must hold fileMutex_.
  OpenFileResult openEncoderForNextFile();
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

  std::unique_ptr<float[]> inputBufferPool_;
  size_t samplesPerSlot_{0};
  std::unique_ptr<FreeList> freeSlots_;
  std::unique_ptr<Offloader> offloader_;
};

} // namespace audioapi
