#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/encoding/AudioEncoder.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SlotFreeList.hpp>
#include <audioapi/utils/SpscChannel.hpp>
#include <audioapi/utils/TaskOffloader.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>

namespace audioapi {

class AudioFileProperties;
class IAudioEventHandlerRegistry;

/// Recorder file writer backed by the platform's system encoder. The audio thread copies
/// each callback into a preallocated slot; a worker thread encodes it. Marked final because
/// RotatingFileWriter takes any AudioFileWriter from its factory — a writer that could be
/// specialised into a rotating one would let a factory nest rotation inside itself.
/// Slot index plus frame count — the only thing that crosses to the worker thread.
/// The default slot doubles as TaskOffloader's shutdown sentinel. Deliberately at
/// namespace scope: nested in the writer, the default member initializers would not
/// be usable while the enclosing class is still incomplete, so TaskOffloader's
/// std::default_initializable constraint could not be satisfied.
struct PendingFileWrite {
  size_t slot = std::numeric_limits<size_t>::max();
  int numFrames = 0;
};

class EncodedAudioFileWriter final : public AudioFileWriter {
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

  void writeAudioData(const float *interleavedFrames, int numFrames) override;

  [[nodiscard]] std::string getFilePath() const override;
  [[nodiscard]] double getCurrentDuration() const override;
  [[nodiscard]] size_t getFileSizeBytes() const override;

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

  bool initializePreallocatedInputPool();
  void cleanupPreallocatedInputPool();
  void createOffloader();
  void runWriterTask(PendingFileWrite pending);
  void rollbackFailedOpen();

  float streamSampleRate_{0.0F};
  int32_t streamChannelCount_{0};
  int32_t maxFramesPerBuffer_{0};
  std::string filePath_;

  std::unique_ptr<AudioEncoder> encoder_;
  std::unique_ptr<float[]> inputBufferPool_;
  size_t samplesPerSlot_{0};
  std::unique_ptr<FreeList> freeSlots_;

  // Lifetime tied to the preallocated pool so a writer can be reopened after closeFile().
  std::unique_ptr<Offloader> offloader_;
};

} // namespace audioapi
