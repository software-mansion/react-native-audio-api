#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SlotFreeList.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

namespace audioapi {

class AudioFileProperties;
class IAudioEventHandlerRegistry;

/// Slot index plus frame count — the only thing that crosses to the worker thread.
/// The default slot doubles as TaskOffloader's shutdown sentinel. Deliberately at
/// namespace scope: nested in the writer, the default member initializers would not
/// be usable while the enclosing class is still incomplete, so TaskOffloader's
/// std::default_initializable constraint could not be satisfied.
struct PendingFileWrite {
  size_t slot = std::numeric_limits<size_t>::max();
  int numFrames = 0;
};

/// Recorder file writer backed by the platform's system encoder. The audio thread copies each
/// callback into a preallocated slot; a worker thread encodes it. That worker is created once
/// per session and outlives switchToFile(), so rotating a recording costs no thread at all.
///
/// RotatingFileWriter reaches switchToFile() through a dynamic_cast to this type, so a subclass
/// answers that cast as well. Deriving is for test doubles that stand in for the platform
/// encoder; do not specialise this into a rotating writer, which would nest rotation inside
/// itself.
class EncodedAudioFileWriter : public AudioFileWriter {
 public:
  EncodedAudioFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties);
  ~EncodedAudioFileWriter() override;
  DELETE_COPY_AND_MOVE(EncodedAudioFileWriter);

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer,
      const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

  /// Retires the current file and continues the session into @p fileNameOverride, reusing the
  /// worker thread and buffer pool already in place. Returns the retired file's
  /// {sizeMB, durationSeconds}. Runs on this writer's own worker thread, so it may block on the
  /// encoder. Virtual so a test double can stand in for the platform encoder.
  virtual CloseFileResult switchToFile(const std::string &fileNameOverride);

  /// Registers a listener called on the worker thread after each buffer is encoded, giving
  /// rotation somewhere to run that is neither the audio thread nor a thread of its own.
  /// Must be set before openFile(); the worker reads it without synchronization.
  void setOnBufferEncodedCallback(std::function<void()> callback);

  void writeAudioData(const float *interleavedFrames, int numFrames) override;

  [[nodiscard]] std::string getFilePath() const override;
  [[nodiscard]] double getCurrentDuration() const override;
  [[nodiscard]] size_t getFileSizeBytes() const override;

 protected:
  /// Announces that one buffer has been encoded, which is where rotation gets to run.
  /// Called on the worker thread, and by test doubles standing in for it.
  void notifyBufferEncoded();

 private:
  static constexpr auto FILE_WRITER_SPSC_OVERFLOW_STRATEGY =
      channels::spsc::OverflowStrategy::OVERWRITE_ON_FULL;
  static constexpr auto FILE_WRITER_SPSC_WAIT_STRATEGY = channels::spsc::WaitStrategy::ATOMIC_WAIT;
  static constexpr size_t FILE_WRITER_POOL_SIZE = 32;
  // SPSC rings hold at most (capacity - 1) elements.
  static constexpr auto FILE_WRITER_CHANNEL_CAPACITY = FILE_WRITER_POOL_SIZE + 1;
  // At most POOL_SIZE slots can be in flight at once, so sizing the channel one
  // larger guarantees the ring is never full when a slot is available — which is
  // why the producer can use the blocking send() without it ever actually waiting.
  static_assert(
      FILE_WRITER_POOL_SIZE <= FILE_WRITER_CHANNEL_CAPACITY - 1,
      "Channel must hold every in-flight slot so send() never blocks/overwrites");

  using FreeList = slots::SlotFreeList<FILE_WRITER_POOL_SIZE>;
  using Offloader = task_offloader::TaskOffloader<
      PendingFileWrite,
      FILE_WRITER_SPSC_OVERFLOW_STRATEGY,
      FILE_WRITER_SPSC_WAIT_STRATEGY>;

  /// Resolves the path for @p fileNameOverride and opens an encoder on it.
  /// The caller must hold fileMutex_.
  OpenFileResult openEncoderForFile(const std::string &fileNameOverride);
  /// Closes and releases the encoder and zeroes the frame count, returning what the encoder
  /// reported for the file it finished. The one place "retiring an encoder" is defined; the
  /// three callers differ only in what they do with the path afterwards.
  /// The caller must hold fileMutex_.
  CloseEncoderResult retireEncoder();
  bool initializePreallocatedInputPool();
  void cleanupPreallocatedInputPool();
  void createOffloader();
  void runWriterTask(PendingFileWrite pending);
  void rollbackFailedOpen();

  float streamSampleRate_{0.0F};
  int32_t streamChannelCount_{0};
  int32_t maxFramesPerBuffer_{0};

  /// Guards the encoder and the path it writes to, which switchToFile() swaps on the worker
  /// thread while the JS thread reads them. Never taken on the audio thread.
  mutable std::mutex fileMutex_;
  std::string filePath_;
  std::unique_ptr<AudioEncoder> encoder_;

  std::function<void()> onBufferEncoded_;
  std::unique_ptr<float[]> inputBufferPool_;
  size_t samplesPerSlot_{0};
  std::unique_ptr<FreeList> freeSlots_;

  // Lifetime tied to the preallocated pool so a writer can be reopened after closeFile().
  std::unique_ptr<Offloader> offloader_;
};

} // namespace audioapi
