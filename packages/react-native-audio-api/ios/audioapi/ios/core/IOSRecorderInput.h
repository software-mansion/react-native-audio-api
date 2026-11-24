#pragma once

#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <memory>
#include <string>

namespace audioapi {

class RecorderInputGate;
struct AudioRecorderOptions;

/// @brief The shared AudioEngine's input node, as the recorder's capture stream.
///
/// iOS reroutes capture itself: on a route change the engine rebuilds its graph and reports it,
/// and this input forwards that as RecorderInputEvents::formatChanged(). So acquireRoute() holds no
/// route. Attaching the input node is only allowed with the engine stopped, and the engine has to
/// start again right after, so open() does all three and start() only makes sure the engine runs.
class IOSRecorderInput final : public IRecorderInput,
                               public std::enable_shared_from_this<IOSRecorderInput> {
 public:
  explicit IOSRecorderInput(const AudioRecorderOptions &options);
  ~IOSRecorderInput() override;
  DELETE_COPY_AND_MOVE(IOSRecorderInput);

  Result<NoneType, std::string> open(
      RouteLease route,
      InputOpenPurpose purpose,
      std::shared_ptr<RecorderInputGate> gate,
      RecorderInputEvents events) override;
  Result<NoneType, std::string> start() override;
  void pause() override;
  Result<NoneType, std::string> resume() override;
  void close() override;

  [[nodiscard]] Result<StreamFormat, std::string> getCurrentFormat() const override;
  [[nodiscard]] bool isRunning() const override;
  [[nodiscard]] double latency(double callbackSeconds) const override;

 private:
  const bool voiceProcessingEnabled_;

  /// Whether open() attached the input node, which close() detaches. Written on the recorder's
  /// control thread; isRunning() reads it from any thread.
  std::atomic<bool> isOpen_{false};
};

} // namespace audioapi
