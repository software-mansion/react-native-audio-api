#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <utility>

namespace audioapi {

/// @brief An input route the platform set up for a capture stream, given back when the lease is
/// destroyed.
///
/// Taking a route can block for seconds (Android waits for a Bluetooth headset's SCO link), so the
/// recorder takes it with no lock held. If the session ends while it waits, dropping the lease is
/// all it takes to give the route back.
class RouteLease {
 public:
  RouteLease() = default;

  /// @p deviceId is the device to open the capture stream on, or nullopt when the platform could
  /// not route the selected input. @p release gives the route back; it runs exactly once.
  RouteLease(std::optional<int32_t> deviceId, std::function<void()> release)
      : deviceId_(deviceId), release_(std::move(release)) {}

  RouteLease(RouteLease &&other) noexcept
      : deviceId_(other.deviceId_), release_(std::exchange(other.release_, nullptr)) {}

  RouteLease &operator=(RouteLease &&other) noexcept {
    if (this != &other) {
      giveBack();
      deviceId_ = other.deviceId_;
      release_ = std::exchange(other.release_, nullptr);
    }
    return *this;
  }

  RouteLease(const RouteLease &) = delete;
  RouteLease &operator=(const RouteLease &) = delete;

  ~RouteLease() {
    giveBack();
  }

  [[nodiscard]] const std::optional<int32_t> &deviceId() const {
    return deviceId_;
  }

 private:
  void giveBack() {
    if (auto release = std::exchange(release_, nullptr)) {
      release();
    }
  }

  std::optional<int32_t> deviceId_;
  std::function<void()> release_;
};

} // namespace audioapi
