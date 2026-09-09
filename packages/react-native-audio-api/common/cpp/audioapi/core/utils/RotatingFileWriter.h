#pragma once

#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/utils/AudioFileProperties.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

namespace audioapi {

class EncodedAudioFileWriter;

/// Splits a recording into size-capped segments. Owns no encoder and no thread of its own: it
/// drives a single @p segmentWriter, deciding when that writer should move on to the next file
/// and accumulating size and duration across the files it leaves behind.
///
/// The deciding happens on the segment writer's worker thread, from its buffer-encoded
/// listener, never in writeAudioData(). Measuring a file costs a stat and rotating one costs a
/// flush and an open, and the audio thread can afford neither.
class RotatingFileWriter final : public AudioFileWriter {
 public:
  using OnSegmentFileOpenedCallback = std::function<void(const std::string &)>;

  RotatingFileWriter(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const std::shared_ptr<AudioFileProperties> &fileProperties,
      size_t rotateIntervalBytes,
      std::shared_ptr<AudioFileWriter> segmentWriter,
      OnSegmentFileOpenedCallback onSegmentFileOpened = {});

  OpenFileResult openFile(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer,
      const std::string &fileNameOverride) override;
  CloseFileResult closeFile() override;

  void writeAudioData(const float *interleavedFrames, int numFrames) override;

  /// Closes the current segment like a rotation — folding its size and duration into the
  /// cumulative totals — and opens the next one with the new stream format.
  OpenFileResult reprepareStreamFormat(
      float streamSampleRate,
      int32_t streamChannelCount,
      int32_t maxFramesPerBuffer);

  [[nodiscard]] std::string getFilePath() const override;
  [[nodiscard]] double getCurrentDuration() const override;
  [[nodiscard]] size_t getFileSizeBytes() const override;

  /// Errors are raised by whichever writer hits them, so the segment writer needs the
  /// callback just as much as this one.
  void assignOnErrorCallbackId(uint64_t callbackId) override;

 private:
  /// Checking the file size costs a stat(), so only do it every Nth write.
  static constexpr int FILE_SIZE_CHECK_WRITE_INTERVAL = 10;

  /// Worker thread. Measures the current segment every Nth buffer and starts the next one
  /// once it has outgrown the cap.
  void onSegmentWriterBufferEncoded();
  OpenFileResult openNextSegment();
  /// Claims and names the next segment, numbered from 1.
  std::string nextSegmentStem();
  void foldRetiredSegment(const std::tuple<double, double> &retired);
  void announceSegmentOpened();

  OnSegmentFileOpenedCallback onSegmentFileOpened_;
  size_t rotateIntervalBytes_;
  /// Advanced by the worker thread. The JS thread resets it only while no worker is running:
  /// before openFile() creates one, and after reprepareStreamFormat() has joined it.
  size_t writesSinceLastCheck_ = 0;

  /// Guards the session bookkeeping below, which the worker thread advances on every rotation
  /// while the JS thread reads it. Never taken on the audio thread.
  mutable std::mutex segmentMutex_;
  std::string sessionStem_;
  size_t segmentIndex_ = 0;
  double cumulativeSizeMB_{0.0};
  double cumulativeDurationSec_{0.0};

  float streamSampleRate_{0.0F};
  int32_t streamChannelCount_{0};
  int32_t maxFramesPerBuffer_{0};

  /// The segment writer again, as the type that can hand off to the next file. Non-owning:
  /// segmentWriter_ below owns it. Null when the writer cannot rotate.
  EncodedAudioFileWriter *rotatable_ = nullptr;
  /// Declared last so it is destroyed first. Its destructor joins the worker thread, and that
  /// worker calls back into this object — so every member it touches must still be alive.
  std::shared_ptr<AudioFileWriter> segmentWriter_;
};

} // namespace audioapi
