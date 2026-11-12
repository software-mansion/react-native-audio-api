#pragma once

#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace audioapi {

class RecorderInputGate;
struct AudioRecorderOptions;

/// @brief The shared AudioEngine's input node, as the recorder's capture stream.
///
/// iOS reroutes capture itself: on a route change the engine rebuilds its graph and reports it,
/// and this input forwards that as RecorderInputEvents::formatChanged(). So acquireRoute() holds no
/// route, and opening starts the engine, which is why start() has nothing left to do.
class IOSRecorderInput final : public IRecorderInput,
                               public std::enable_shared_from_this<IOSRecorderInput> {
 public:
  explicit IOSRecorderInput(const AudioRecorderOptions &options);
  ~IOSRecorderInput() override;
  DELETE_COPY_AND_MOVE(IOSRecorderInput);

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

 private:
  /// Whether the calling thread is inside one of this input's own engine calls; see
  /// engineCallThread_. Engine notification threads.
  [[nodiscard]] bool isInsideOwnEngineCall() const;

  const bool voiceProcessingEnabled_;

  /// Written on the recorder's control thread; isRunning() reads it from any thread.
  std::atomic<bool> isOpen_{false};

  /// The thread inside an AudioEngine call made on the recorder's behalf, if any. The engine
  /// reports input changes synchronously from inside such calls (startIfNecessary rebuilds the
  /// graph first when the hardware changed), while the recorder holds its control mutex. Forwarding
  /// from that thread would deadlock, and is not needed: the recorder reads the format right after
  /// the call returns.
  std::atomic<std::thread::id> engineCallThread_{};
};

} // namespace audioapi
