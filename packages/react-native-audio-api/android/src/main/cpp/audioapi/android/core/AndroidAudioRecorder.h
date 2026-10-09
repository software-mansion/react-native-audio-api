#pragma once

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <oboe/Oboe.h>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace audioapi {

class AudioFileProperties;
class AndroidRecorderCallback;
class IAudioEventHandlerRegistry;

class AndroidAudioRecorder : public oboe::AudioStreamCallback,
                             public AudioRecorder,
                             public std::enable_shared_from_this<AndroidAudioRecorder> {
 public:
  explicit AndroidAudioRecorder(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      AudioRecorderOptions options = {});
  ~AndroidAudioRecorder() override;
  void cleanup();

  DELETE_COPY_AND_MOVE(AndroidAudioRecorder);

  Result<NoneType, std::string> start() override;
  Result<FileInfo, std::string> stop() override;

  void pause() override;
  void resume() override;
  /// @brief Reopens the capture stream on the input MediaSessionManager currently selects.
  /// A stream format that differs from the previous one re-points the outputs at it without
  /// splitting the file (see reprepareOutputs()). Does nothing while idle.
  Result<NoneType, std::string> rerouteInput() override;
  bool isRecording() const override;
  bool isPaused() const override;
  bool isIdle() const override;

  [[nodiscard]] double getInputLatency() const override;

  oboe::DataCallbackResult
  onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;
  void onErrorAfterClose(oboe::AudioStream *oboeStream, oboe::Result error) override;

 protected:
  /// Oboe resolves the format once the stream is open, so the cached values stay valid for
  /// the whole session; a disconnect closes the stream and refreshes them on reopen.
  [[nodiscard]] Result<StreamFormat, std::string> resolveStreamFormat() const override;

 private:
  /// The caller holds streamMutex_ and an input route (see adoptInputRoute()). A stream that
  /// is open on a device other than the route's is closed and reopened.
  Result<NoneType, std::string> openAudioStream();
  /// Closes the stream and leaves the recorder's state alone, so a restart can reopen it
  /// mid-session. The caller holds streamMutex_.
  void closeStream();
  /// The caller holds streamMutex_.
  [[nodiscard]] bool isStreamRecording() const;
  /// @brief Opens the stream on the adopted route and starts it when @p targetState is
  /// Recording. @p preparedFormat is nullopt for a new session, whose outputs are prepared
  /// from scratch; for a live one it is the format its outputs were prepared for, and they
  /// follow the stream only when its format differs.
  ///
  /// The caller holds callbackMutex_, fileWriterMutex_, adapterNodeMutex_ and streamMutex_.
  Result<NoneType, std::string> startCapture(
      const std::optional<StreamFormat> &preparedFormat,
      RecorderState targetState);
  /// @brief Closes the capture stream, opens a new one on the currently selected input and
  /// carries the recording or paused session over to it.
  ///
  /// Call with none of the recorder's mutexes held.
  Result<NoneType, std::string> restartCapture();
  /// @brief Records a route the caller took with NativeInputRouting::acquireInputRoute(), which
  /// openAudioStream() then opens on. acquireInputRoute() itself can block for seconds, so it is
  /// called with none of the recorder's mutexes held. The caller holds streamMutex_.
  void adoptInputRoute(std::optional<int32_t> routedDeviceId);
  /// @brief Hands the route back; does nothing when none is held. The caller holds streamMutex_.
  void releaseInputRoute();

  /// Taken once by each entry point (the public methods and Oboe's error callback); the
  /// private stream helpers above never take it.
  mutable std::mutex streamMutex_;

  AudioRecorderOptions::AndroidInputPreset inputPreset_;
  int32_t streamChannelCount_{0};
  int32_t streamMaxBufferSizeInFrames_{0};
  /// The device mStream_ was opened for. Guarded by streamMutex_.
  int32_t streamDeviceId_;
  /// Whether this recorder holds a route from NativeInputRouting::acquireInputRoute, and that
  /// route's device. Guarded by streamMutex_.
  bool holdsInputRoute_{false};
  int32_t captureDeviceId_;

  /// Oboe delivers interleaved float32 and every consumer takes planar, so each callback is
  /// repacked here once. Sized under streamMutex_ before the stream starts.
  AudioBuffer planarInput_;

  std::shared_ptr<oboe::AudioStream> mStream_;
};

} // namespace audioapi
