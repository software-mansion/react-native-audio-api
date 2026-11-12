#include <audioapi/core/inputs/RouteLease.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <utility>

using namespace audioapi;

// NOLINTBEGIN

namespace {

RouteLease countingLease(int &releases, std::optional<int32_t> deviceId = 7) {
  return {deviceId, [&releases] { ++releases; }};
}

} // namespace

TEST(RouteLeaseTest, DestroyingALeaseGivesTheRouteBackOnce) {
  int releases = 0;
  {
    auto lease = countingLease(releases);
    EXPECT_EQ(lease.deviceId(), std::optional<int32_t>(7));
  }

  EXPECT_EQ(releases, 1);
}

TEST(RouteLeaseTest, AMovedLeaseGivesTheRouteBackOnlyFromItsNewOwner) {
  int releases = 0;
  {
    auto first = countingLease(releases);
    RouteLease second(std::move(first));
    EXPECT_EQ(releases, 0);
  }

  EXPECT_EQ(releases, 1);
}

TEST(RouteLeaseTest, AssigningOverALeaseGivesItsRouteBackFirst) {
  int firstReleases = 0;
  int secondReleases = 0;
  auto held = countingLease(firstReleases);

  held = countingLease(secondReleases, std::nullopt);

  EXPECT_EQ(firstReleases, 1);
  EXPECT_EQ(secondReleases, 0);
  EXPECT_FALSE(held.deviceId().has_value());

  held = RouteLease{};
  EXPECT_EQ(secondReleases, 1);
}

TEST(RouteLeaseTest, AnEmptyLeaseGivesNothingBack) {
  RouteLease lease;

  EXPECT_FALSE(lease.deviceId().has_value());
}

// NOLINTEND
