#pragma once

#include <audioapi/core/inputs/RecorderInputEvents.h>
#include <audioapi/core/inputs/RouteLease.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/Result.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace audioapi {

class RecorderInputGate;
struct AudioRecorderOptions;

/// Why AudioRecorder opens its input.
enum class InputOpenPurpose : uint8_t {
  /// start(): failing to route the selected input fails the start.
  StartSession,
  /// A reroute of a live session: when the selected input cannot be routed, capture continues on
  /// the platform's default input rather than ending the recording.
  ContinueSession,
};

/// @brief The platform's microphone capture stream, driven by AudioRecorder.
///
/// The recorder calls every method except isRunning() and latency() with its control mutex held,
/// and acquireRoute() with no lock held, since it may block. While open, the input hands frames to
/// the RecorderInputGate open() gave it, and reports changes it did not cause through the
/// RecorderInputEvents open() gave it. It never reports them from inside one of the calls below:
/// the recorder holds its control mutex there, and re-reads the format afterwards anyway.
class IRecorderInput {
 public:
  virtual ~IRecorderInput() = default;

  virtual RouteLease acquireRoute() {
    return {};
  }

  /// Opens capture on @p route, delivering frames to @p gate and reporting to @p events until
  /// close(). The input may deliver frames as soon as this returns; the gate drops them until the
  /// recorder arms it. On failure the input stays closed and the route is given back.
  virtual Result<StreamFormat, std::string> open(
      RouteLease route,
      InputOpenPurpose purpose,
      std::shared_ptr<RecorderInputGate> gate,
      RecorderInputEvents events) = 0;

  virtual Result<NoneType, std::string> start() = 0;
  virtual void pause() = 0;
  virtual Result<NoneType, std::string> resume() = 0;
  virtual void close() = 0;

  /// The format the open input delivers now. It changes when the platform reconfigures the input
  /// on its own, as iOS does on a route change. Fails while the platform has not settled on one.
  [[nodiscard]] virtual Result<StreamFormat, std::string> currentFormat() const = 0;
  /// Whether capture is started: false before start(), after pause() and close(), and when the
  /// platform stopped the input on its own (an interrupted iOS session). Any thread.
  [[nodiscard]] virtual bool isRunning() const = 0;
  [[nodiscard]] virtual double latency(double callbackSeconds) const = 0;
};

std::shared_ptr<IRecorderInput> createPlatformRecorderInput(const AudioRecorderOptions &options);

} // namespace audioapi
