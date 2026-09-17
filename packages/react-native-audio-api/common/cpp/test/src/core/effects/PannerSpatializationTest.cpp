#include <audioapi/core/effects/PannerSpatialization.h>
#include <gtest/gtest.h>

using namespace audioapi;
using namespace audioapi::panner;

TEST(PannerSpatializationTest, WrapAzimuth) {
  EXPECT_DOUBLE_EQ(wrapAzimuth(120.0), 60.0);
  EXPECT_DOUBLE_EQ(wrapAzimuth(-120.0), -60.0);
  EXPECT_DOUBLE_EQ(wrapAzimuth(90.0), 90.0);
  EXPECT_DOUBLE_EQ(wrapAzimuth(-90.0), -90.0);
  EXPECT_DOUBLE_EQ(wrapAzimuth(0.0), 0.0);
  EXPECT_DOUBLE_EQ(wrapAzimuth(180.0), 0.0);
  EXPECT_DOUBLE_EQ(wrapAzimuth(-180.0), 0.0);
}

TEST(PannerSpatializationTest, ClampAzimuth) {
  EXPECT_DOUBLE_EQ(clampAzimuth(200.0), 180.0);
  EXPECT_DOUBLE_EQ(clampAzimuth(-200.0), -180.0);
  EXPECT_DOUBLE_EQ(clampAzimuth(45.0), 45.0);
}

TEST(PannerSpatializationTest, ComputeAzimuth) {
  const Vec3 listenerPos{.x = 0.0, .y = 0.0, .z = 0.0};
  const Vec3 listenerFwd{.x = 0.0, .y = 0.0, .z = -1.0};
  const Vec3 listenerUp{.x = 0.0, .y = 1.0, .z = 0.0};

  constexpr double TOLERANCE = 1e-9;

  EXPECT_NEAR(
      computeAzimuth({0.0, 0.0, -1.0}, listenerPos, listenerFwd, listenerUp), 0.0, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({1.0, 0.0, 0.0}, listenerPos, listenerFwd, listenerUp), 90.0, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({-1.0, 0.0, 0.0}, listenerPos, listenerFwd, listenerUp), -90.0, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({0.0, 0.0, 0.0}, listenerPos, listenerFwd, listenerUp), 0.0, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({0.0, 0.0, 1.0}, listenerPos, listenerFwd, listenerUp), -180.0, TOLERANCE);
  // Source directly above the listener has no horizontal projection → azimuth 0.
  EXPECT_NEAR(
      computeAzimuth({0.0, 1.0, 0.0}, listenerPos, listenerFwd, listenerUp), 0.0, TOLERANCE);
}

TEST(PannerSpatializationTest, ComputeDistance) {
  EXPECT_DOUBLE_EQ(computeDistance({0.0, 0.0, 0.0}, {3.0, 4.0, 0.0}), 5.0);
  EXPECT_DOUBLE_EQ(computeDistance({1.0, 1.0, 1.0}, {1.0, 1.0, 1.0}), 0.0);
}

TEST(PannerSpatializationTest, ComputeDistanceGain) {
  constexpr double maxDist = 10000.0;

  EXPECT_DOUBLE_EQ(computeDistanceGain(DistanceModelType::Inverse, 10.0, 1.0, maxDist, 1.0), 0.1);

  EXPECT_DOUBLE_EQ(computeDistanceGain(DistanceModelType::Inverse, 10.0, 0.0, maxDist, 1.0), 0.0);

  EXPECT_DOUBLE_EQ(computeDistanceGain(DistanceModelType::Linear, 5.0, 1.0, 9.0, 1.0), 0.5);

  EXPECT_DOUBLE_EQ(
      computeDistanceGain(DistanceModelType::Exponential, 2.0, 1.0, maxDist, 1.0), 0.5);
}

TEST(PannerSpatializationTest, ComputeConeGain) {
  const Vec3 listenerPos{.x = 0.0, .y = 0.0, .z = 0.0};
  const Vec3 sourcePos{.x = 0.0, .y = 0.0, .z = -1.0};

  constexpr double SILENCE = 0.0;
  constexpr double FULL_GAIN = 1.0;

  // 360/360 cone angles act as a bypass (omnidirectional) => fully audible
  const Vec3 orientationRight{.x = 1.0, .y = 0.0, .z = 0.0};
  EXPECT_DOUBLE_EQ(
      computeConeGain(sourcePos, listenerPos, orientationRight, 360.0, 360.0, 0.0), FULL_GAIN);

  // Source is facing front, listener in the inner angle => fully audible
  const Vec3 orientationFront{.x = 0.0, .y = 0.0, .z = 1.0};
  EXPECT_DOUBLE_EQ(
      computeConeGain(sourcePos, listenerPos, orientationFront, 60.0, 90.0, 0.0), FULL_GAIN);

  // Source is facing back, listener outside the outer angle => silent
  const Vec3 orientationBack{.x = 0.0, .y = 0.0, .z = -1.0};
  EXPECT_DOUBLE_EQ(
      computeConeGain(sourcePos, listenerPos, orientationBack, 60.0, 90.0, 0.0), SILENCE);

  // Interpolation gain equal to 0.5. Angle is 37.5 degrees from the listener.
  const double angle = 37.5 * PI / DEG_180;
  Vec3 orientationLerp{.x = std::sin(angle), .y = 0.0, .z = std::cos(angle)};
  constexpr double TOLERANCE = 1e-12;
  EXPECT_NEAR(
      computeConeGain(sourcePos, listenerPos, orientationLerp, 60.0, 90.0, 0.0), 0.5, TOLERANCE);

  const Vec3 orientationZero{.x = 0.0, .y = 0.0, .z = 0.0};
  EXPECT_DOUBLE_EQ(
      computeConeGain(sourcePos, listenerPos, orientationZero, 60.0, 90.0, 0.0), FULL_GAIN);
}
