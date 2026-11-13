#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace audioapi {

struct RouteReleaser {
  std::function<void()> release;

  void operator()(std::optional<int32_t> *deviceId) const {
    release();
    std::default_delete<std::optional<int32_t>>()(deviceId);
  }
};

/// @brief An input route the platform set up for a capture stream, given back when the lease is
/// destroyed. Holds the device to open the stream on, or nullopt when the platform could not
/// route the selected input; the route is given back either way.
///
/// Taking a route can block for seconds (Android waits for a Bluetooth headset's SCO link), so the
/// recorder takes it with no lock held. If the session ends while it waits, dropping the lease is
/// all it takes to give the route back.
using RouteLease = std::unique_ptr<std::optional<int32_t>, RouteReleaser>;

} // namespace audioapi
