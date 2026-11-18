#pragma once

#include <audioapi/core/inputs/RecorderOutput.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioBufferOffloader.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace audioapi {

class AudioFileProperties;

using OpenFileResult = Result<std::string, std::string>;

struct ClosedSession {
  /// Every file the session produced, in open order.
  std::vector<std::string> filePaths;
  double sizeMB = 0.0;
  double durationSec = 0.0;
};

using CloseFileResult = Result<ClosedSession, std::string>;

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

/// Writes a recording session to one file, or to a series of them when the properties rotate
/// files. A RecorderOutput, so the recorder feeds and repoints it like any other, but closed
/// through closeFile(), which reports the session's files.
class AudioFileWriter final : public RecorderOutput {
 public:
  /// Reports its errors through @p errorEvent, the recorder's.
  AudioFileWriter(
      std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      const PlatformFileBackend &backend = osFileBackend());
  /// The writer borrows @p backend, so a temporary would dangle.
  AudioFileWriter(
      std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      PlatformFileBackend &&backend) = delete;
  ~AudioFileWriter();
  DELETE_COPY_AND_MOVE(AudioFileWriter);

  /// Opens the session's first file. @p streamFormat's maxFramesPerBuffer bounds a single
  /// write(). Fails while a session is open, until closeFile() ends it.
  Result<NoneType, std::string> prepare(const StreamFormat &streamFormat) override;

  /// Sizes and durations are summed over every file of the session. A session whose current file
  /// a failed format change or rotation already finished is still reported, with the files it got
  /// to; fails only when no session is open.
  CloseFileResult closeFile();
  /// Ends the session without reporting it; closeFile() is what hands its files on.
  void close() override;

  /// Keeps the open file and re-points its encoder at @p streamFormat. Drains the worker first,
  /// so the caller must already have stopped writing in the old format. On failure the file is
  /// finished as far as it got.
  Result<NoneType, std::string> reprepare(const StreamFormat &streamFormat) override;

  /// Never blocks: queues the frames for the encoder's worker, dropping them when no pool slot
  /// is free. Encode and rotation errors happen on that worker, so they are reported as the
  /// recorder's error event from there and this always succeeds.
  Result<NoneType, std::string> write(const AudioBuffer &frames, size_t numFrames) override;

  [[nodiscard]] std::string getFilePath() const;
  /// Every file the current session has opened so far, in open order.
  [[nodiscard]] std::vector<std::string> getSessionFilePaths() const;
  /// Across every file of the session.
  [[nodiscard]] double getCurrentDuration() const;
  [[nodiscard]] size_t getFileSizeBytes() const;

 private:
  /// Checking the file size costs a stat(), so only do it every Nth encoded buffer.
  static constexpr int FILE_SIZE_CHECK_WRITE_INTERVAL = 10;

  [[nodiscard]] bool isFileOpen() const;
  [[nodiscard]] bool rotatesFiles() const;

  /// With no worker running: also sizes the buffer pool and starts the worker.
  OpenFileResult startNextFile(const StreamFormat &streamFormat);
  /// JS thread. Joins the worker, then closes the current file into the session totals. Ok,
  /// adding nothing, when no file is left to close.
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

  void runWriterTask(PendingFrames pending);

  std::shared_ptr<AudioFileProperties> fileProperties_;
  /// Declared before worker_, so the worker thread that calls into it is joined first.
  /// Borrowed; must outlive the writer.
  const PlatformFileBackend &backend_;
  std::shared_ptr<EventCaller<AudioEvent::RECORDER_ERROR>> errorEvent_;

  std::atomic<bool> isFileOpen_{false};

  StreamFormat streamFormat_{};

  /// Guards the members below, which a rotation advances on the worker thread while the JS
  /// thread reads them. Never taken on the audio thread.
  mutable std::mutex fileMutex_;
  /// Open on the current file; nullptr between files, and after a rotation fails.
  std::unique_ptr<AudioEncoder> currentEncoder_;
  /// From a successful prepare() until closeFile(): there is a session to report, even after a
  /// failed format change or rotation finished its current file.
  bool isSessionOpen_{false};
  std::string sessionStem_;
  std::vector<std::string> sessionFilePaths_;
  size_t openedFileCount_{0};
  int writesSinceLastSizeCheck_{0};
  double finishedFilesSizeMB_{0.0};
  double finishedFilesDurationSec_{0.0};

  /// Carries audio-thread buffers in streamFormat_ to the encoder. Last, so its worker is joined
  /// before anything it calls into goes.
  AudioBufferOffloader worker_{
      [this](PendingFrames pending) { runWriterTask(std::move(pending)); }};
};

} // namespace audioapi
