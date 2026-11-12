#pragma once

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecorderInputGate.h>
#include <audioapi/core/utils/AudioFileWriter.h>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// NOLINTBEGIN

namespace audioapi::test_support {

/// Scripted stand-in for the platform capture stream. The test sets its fields between recorder
/// calls and plays the audio thread through deliverFrames().
class FakeRecorderInput final : public IRecorderInput {
 public:
  /// What open() reports and currentFormat() returns.
  StreamFormat format{
      .layout = {.sampleRate = 48000.0F, .channelCount = 1},
      .maxFramesPerBuffer = 256,
  };
  bool formatUnavailable = false;

  std::optional<std::string> openError;
  std::optional<std::string> startError;
  std::optional<std::string> resumeError;

  /// The device the next acquired route reports; nullopt when the route could not be set up.
  std::optional<int32_t> routedDeviceId = 0;
  /// Runs once inside the next acquireRoute(), where the recorder holds no lock.
  std::function<void()> duringAcquireRoute;

  std::atomic<int> routesAcquired{0};
  std::atomic<int> routesReleased{0};
  std::atomic<int> closeCount{0};
  std::vector<InputOpenPurpose> openPurposes;
  /// Those of the last open(), kept after close(), so a test can replay a late notification.
  std::optional<RecorderInputEvents> events;
  bool isOpen = false;
  std::atomic<bool> running{false};

  RouteLease acquireRoute() override {
    routesAcquired.fetch_add(1);
    if (auto hook = std::exchange(duringAcquireRoute, nullptr)) {
      hook();
    }
    return {routedDeviceId, [this] { routesReleased.fetch_add(1); }};
  }

  Result<StreamFormat, std::string> open(
      RouteLease route,
      InputOpenPurpose purpose,
      std::shared_ptr<RecorderInputGate> gate,
      RecorderInputEvents openEvents) override {
    openPurposes.push_back(purpose);
    events = openEvents;
    if (openError.has_value()) {
      return Result<StreamFormat, std::string>::Err(*openError);
    }
    gate_ = std::move(gate);
    route_ = std::move(route);
    isOpen = true;
    return Result<StreamFormat, std::string>::Ok(format);
  }

  Result<NoneType, std::string> start() override {
    if (startError.has_value()) {
      return Result<NoneType, std::string>::Err(*startError);
    }
    running.store(true);
    return Result<NoneType, std::string>::Ok(None);
  }

  void pause() override {
    running.store(false);
  }

  Result<NoneType, std::string> resume() override {
    if (resumeError.has_value()) {
      return Result<NoneType, std::string>::Err(*resumeError);
    }
    running.store(true);
    return Result<NoneType, std::string>::Ok(None);
  }

  void close() override {
    if (!isOpen) {
      return;
    }
    isOpen = false;
    running.store(false);
    closeCount.fetch_add(1);
    route_ = RouteLease{};
  }

  [[nodiscard]] Result<StreamFormat, std::string> currentFormat() const override {
    if (formatUnavailable) {
      return Result<StreamFormat, std::string>::Err("format unavailable");
    }
    return Result<StreamFormat, std::string>::Ok(format);
  }

  [[nodiscard]] bool isRunning() const override {
    return running.load();
  }

  [[nodiscard]] double latency(double callbackSeconds) const override {
    return callbackSeconds;
  }

  /// Plays one audio-thread callback of @p numFrames frames; @p channelCount defaults to the
  /// current format's. Unlike a platform input, the fake keeps the gate after close(), so frames
  /// delivered after a stop still exercise the gate. Before the first open() there is none.
  void deliverFrames(int numFrames, std::optional<int> channelCount = std::nullopt) {
    if (gate_ == nullptr) {
      return;
    }
    const int channels = channelCount.value_or(format.layout.channelCount);
    std::vector<std::vector<float>> samples(
        static_cast<size_t>(channels), std::vector<float>(static_cast<size_t>(numFrames), 0.25F));
    std::vector<const float *> channelPointers;
    for (const auto &channel : samples) {
      channelPointers.push_back(channel.data());
    }
    gate_->deliver(channelPointers.data(), channels, numFrames);
  }

 private:
  std::shared_ptr<RecorderInputGate> gate_;
  RouteLease route_;
};

} // namespace audioapi::test_support

// NOLINTEND
