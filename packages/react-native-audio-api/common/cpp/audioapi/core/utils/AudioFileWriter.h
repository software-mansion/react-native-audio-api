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
#include <vector>

namespace audioapi {

class AudioFileProperties;
class IAudioEventHandlerRegistry;

using OpenFileResult = Result<std::string, std::string>;

struct ClosedSession {
  /// Every file the session produced, in open order.
  std::vector<std::string> filePaths;
  double sizeMB = 0.0;
  double durationSec = 0.0;
};

using CloseFileResult = Result<ClosedSession, std::string>;

/// A filled pool buffer on its way to the worker. The default-constructed value (no buffer)
/// is the offloader's shutdown message, which is what operator== exists for.
struct PendingFileWrite {
  AudioBufferLease buffer;
  int numFrames = 0;

  bool operator==(const PendingFileWrite &) const = default;
};

struct PlatformFileBackend {
  /// Maps the requested file format to the container, codec and file extension this platform
  /// writes it as. Fails when the platform cannot encode that format, before any file exists.
  std::function<Result<EncoderOutputSpec, std::string>(AudioFileProperties::FileFormat)>
      resolveOutputSpec;
  /// Turns a bare file name (stem and extension, no directory) into the absolute path inside
  /// the directory the properties select, creating that directory if it is missing.
  std::function<Result<std::string, std::string>(
      const std::shared_ptr<AudioFileProperties> &,
      const std::string &fileName)>
      resolvePath;
  /// Builds an encoder that is not open yet; the writer opens it on the resolved path. Called
  /// once per file, so a rotating session creates one encoder per segment.
  EncoderFactory createEncoder;
};

/// The iOS and Android backend, built once and shared by every writer.
[[nodiscard]] const PlatformFileBackend &osFileBackend();

class AudioFileWriter final {
 public:
  AudioFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      const PlatformFileBackend &backend = osFileBackend());
  /// The writer borrows @p backend, so a temporary would dangle.
  AudioFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      PlatformFileBackend &&backend) = delete;
  ~AudioFileWriter();
  DELETE_COPY_AND_MOVE(AudioFileWriter);

  /// JS thread. @p streamFormat's maxFramesPerBuffer bounds a single writeAudioData() call.
  /// Returns the opened file's path.
  OpenFileResult openFile(const StreamFormat &streamFormat);

  /// JS thread. Sizes and durations are summed over every file of the session.
  CloseFileResult closeFile();

  /// ios only because android handles input format changes automatically. Returns the file path on success.
  OpenFileResult reprepareStreamFormat(const StreamFormat &streamFormat);

  /// Audio thread. @p channels holds one pointer per stream channel, each to numFrames float32
  /// samples, valid only for the call. Never blocks; drops the buffer when no pool slot is free.
  void writeAudioData(const float *const *channels, int numFrames);

  [[nodiscard]] std::string getFilePath() const;
  /// Every file the current session has opened so far, in open order.
  [[nodiscard]] std::vector<std::string> getSessionFilePaths() const;
  /// Across every file of the session.
  [[nodiscard]] double getCurrentDuration() const;
  [[nodiscard]] size_t getFileSizeBytes() const;

  void setOnErrorCallback(uint64_t callbackId);
  void clearOnErrorCallback();

 private:
  static constexpr auto SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto SPSC_WAIT_STRATEGY = channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr size_t POOL_SIZE = 32;
  // SPSC rings hold at most (capacity - 1) elements.
  static constexpr auto CHANNEL_CAPACITY = POOL_SIZE + 1;
  static_assert(
      POOL_SIZE <= CHANNEL_CAPACITY - 1,
      "Channel must hold every in-flight slot so send() never blocks/overwrites");
  /// Checking the file size costs a stat(), so only do it every Nth encoded buffer.
  static constexpr int FILE_SIZE_CHECK_WRITE_INTERVAL = 10;

  using Offloader =
      task_offloader::TaskOffloader<PendingFileWrite, SPSC_OVERFLOW_STRATEGY, SPSC_WAIT_STRATEGY>;

  [[nodiscard]] bool isFileOpen() const;
  [[nodiscard]] bool rotatesFiles() const;

  /// JS thread, with no worker running: also sizes the buffer pool and starts the worker.
  OpenFileResult startNextFile(const StreamFormat &streamFormat);
  /// JS thread. Joins the worker, then adds the file to the session totals.
  CloseEncoderResult finishCurrentFile();

  /// @p fileNumber is 1-based. A rotated session numbers every file; one that is not has a
  /// single file under the plain stem.
  [[nodiscard]] std::string fileStem(size_t fileNumber) const;
  [[nodiscard]] Result<std::string, std::string> resolveNextFilePath(
      const std::string &stem,
      const std::string &extension) const;
  /// Appends _1, _2, ... to @p stem until the resolved path names no existing file, so a
  /// generated name never overwrites an earlier recording. @p path is the unsuffixed one.
  [[nodiscard]] Result<std::string, std::string>
  firstUnusedPath(const std::string &stem, const std::string &extension, std::string path) const;
  /// The caller must hold fileMutex_.
  OpenFileResult openNextFile();
  /// The caller must hold fileMutex_.
  OpenFileResult changeCurrentFileInputFormat();
  /// The caller must hold fileMutex_.
  CloseEncoderResult closeCurrentFile();
  /// The caller must hold fileMutex_.
  void addFinishedFile(const std::tuple<double, double> &finished);
  /// Closes the current encoder and deletes its file, for a file that must not outlive a
  /// failed open.
  void rollbackFailedOpen();

  /// Worker thread, once per encoded buffer. Swaps the encoder underneath the running worker.
  void rotateIfFileOutgrowsCap();
  void invokeOnErrorCallback(const std::string &message);

  bool initializePreallocatedInputPool();
  void cleanupPreallocatedInputPool();
  void createOffloader();
  void runWriterTask(PendingFileWrite pending);

  std::shared_ptr<AudioFileProperties> fileProperties_;
  /// Declared before offloader_, so the worker thread that calls into it is joined first.
  /// Borrowed; must outlive the writer.
  const PlatformFileBackend &backend_;
  EventCaller<AudioEvent::RECORDER_ERROR> errorEvent_;

  std::atomic<bool> isFileOpen_{false};

  StreamFormat streamFormat_{};

  /// Guards the members below, which a rotation advances on the worker thread while the JS
  /// thread reads them. Never taken on the audio thread.
  mutable std::mutex fileMutex_;
  /// Open on the current file; nullptr between files, and after a rotation fails.
  std::unique_ptr<AudioEncoder> currentEncoder_;
  std::string sessionStem_;
  std::vector<std::string> sessionFilePaths_;
  size_t openedFileCount_{0};
  int writesSinceLastSizeCheck_{0};
  double finishedFilesSizeMB_{0.0};
  double finishedFilesDurationSec_{0.0};

  /// Planar buffers of maxFramesPerBuffer x channelCount of streamFormat_ that carry audio-thread
  /// callbacks to the worker.
  AudioBufferPool<POOL_SIZE> inputBufferPool_;
  std::unique_ptr<Offloader> offloader_;
};

} // namespace audioapi
