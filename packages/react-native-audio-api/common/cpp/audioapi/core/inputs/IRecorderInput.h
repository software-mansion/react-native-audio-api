#pragma once

#include <audioapi/core/inputs/RecorderInputEvents.h>
#include <audioapi/core/inputs/RouteLease.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace audioapi {

class RecorderInputGate;
struct AudioRecorderOptions;

/// Why AudioRecorder opens its input.
enum class InputOpenPurpose : uint8_t {
  StartSession,
  ContinueSession,
};

/// @brief The platform's microphone capture stream, driven by AudioRecorder.
class IRecorderInput {
 public:
  IRecorderInput() = default;
  virtual ~IRecorderInput() = default;
  DELETE_COPY_AND_MOVE(IRecorderInput);

  virtual RouteLease acquireRoute() noexcept {
    return {};
  }

  virtual Result<NoneType, std::string> open(
      RouteLease route,
      InputOpenPurpose purpose,
      std::shared_ptr<RecorderInputGate> gate,
      RecorderInputEvents events) = 0;

  virtual Result<NoneType, std::string> start() = 0;
  virtual void pause() = 0;
  virtual Result<NoneType, std::string> resume() = 0;
  virtual void close() = 0;

  [[nodiscard]] virtual Result<StreamFormat, std::string> getCurrentFormat() const = 0;
  [[nodiscard]] virtual bool isRunning() const = 0;
  [[nodiscard]] virtual double latency(double callbackSeconds) const = 0;
};

std::shared_ptr<IRecorderInput> createPlatformRecorderInput(const AudioRecorderOptions &options);

} // namespace audioapi
