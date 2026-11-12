#pragma once

#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Macros.h>
#include <oboe/Oboe.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace audioapi {

class RecorderInputGate;

/// @brief An Oboe input stream on the device AudioInputRouter selects, as the recorder's capture
/// stream.
class AndroidRecorderInput final : public IRecorderInput,
                                   public oboe::AudioStreamCallback,
                                   public std::enable_shared_from_this<AndroidRecorderInput> {
 public:
  explicit AndroidRecorderInput(const AudioRecorderOptions &options);
  ~AndroidRecorderInput() override;
  DELETE_COPY_AND_MOVE(AndroidRecorderInput);

  /// Asks AudioInputRouter which device to capture from, which can wait seconds for a Bluetooth
  /// headset's SCO link.
  RouteLease acquireRoute() override;
  Result<StreamFormat, std::string> open(
      RouteLease route,
      InputOpenPurpose purpose,
      std::shared_ptr<RecorderInputGate> gate,
      RecorderInputEvents events) override;
  Result<NoneType, std::string> start() override;
  void pause() override;
  Result<NoneType, std::string> resume() override;
  void close() override;

  [[nodiscard]] Result<StreamFormat, std::string> currentFormat() const override;
  [[nodiscard]] bool isRunning() const override;
  [[nodiscard]] double latency(double callbackSeconds) const override;

  /// Audio thread. Oboe delivers interleaved float32 and the recorder takes planar, so each
  /// buffer is repacked here once.
  oboe::DataCallbackResult
  onAudioReady(oboe::AudioStream *stream, void *audioData, int32_t numFrames) override;

  /// Oboe's error thread. A disconnected stream is reported as RecorderInputEvents::lost(), which
  /// reopens it.
  void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result error) override;

 private:
  static constexpr const char *LOG_TAG = "AndroidRecorderInput";

  Result<std::shared_ptr<oboe::AudioStream>, std::string> openStream(int32_t deviceId);

  const AudioRecorderOptions::AndroidInputPreset inputPreset_;

  /// Held from open() to close(). Control thread.
  RouteLease route_;
  /// The open stream's format. Control thread.
  StreamFormat format_{};

  /// Set in open(), before the stream starts, and read by the data callback. close() leaves
  /// them in place: Oboe can deliver one more callback shortly after a stop, even after a close
  /// (see oboe::AudioStream::setDelayBeforeCloseMillis), and a gate that is not armed drops its
  /// frames.
  std::shared_ptr<RecorderInputGate> gate_;
  AudioBuffer planarInput_;
  int channelCount_ = 0;

  /// Written on the control thread only, which reads stream_ without it. Guards stream_ for the
  /// getters and the error thread, which read it off the control thread, and keeps them from
  /// querying a stream that close() is tearing down.
  mutable std::mutex streamMutex_;
  std::shared_ptr<oboe::AudioStream> stream_;
  /// Those of the open() that opened stream_; guarded by streamMutex_ with it.
  std::optional<RecorderInputEvents> events_;

  /// Whether capture is started: set by start() and resume(), cleared by pause() and close().
  /// Not read from the stream, whose own state trails the request to start it.
  std::atomic<bool> isStarted_{false};
};

} // namespace audioapi
