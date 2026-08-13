#include <audioapi/core/effects/PannerSpatialization.h>
#include <gtest/gtest.h>

using namespace audioapi;
using namespace audioapi::panner;

TEST(PannerSpatializationTest, WrapAzimuth) {
  EXPECT_FLOAT_EQ(wrapAzimuth(120.0f), 60.0f);
  EXPECT_FLOAT_EQ(wrapAzimuth(-120.0f), -60.0f);
  EXPECT_FLOAT_EQ(wrapAzimuth(90.0f), 90.0f);
  EXPECT_FLOAT_EQ(wrapAzimuth(-90.0f), -90.0f);
  EXPECT_FLOAT_EQ(wrapAzimuth(0.0f), 0.0f);
  EXPECT_FLOAT_EQ(wrapAzimuth(180.0f), 0.0f);
  EXPECT_FLOAT_EQ(wrapAzimuth(-180.0f), 0.0f);
}

TEST(PannerSpatializationTest, ClampAzimuth) {
  EXPECT_FLOAT_EQ(clampAzimuth(200.0f), 180);
  EXPECT_FLOAT_EQ(clampAzimuth(-200.0f), -1.0f * 180);
  EXPECT_FLOAT_EQ(clampAzimuth(45.0f), 45.0f);
}

TEST(PannerSpatializationTest, ComputeAzimuth) {
  const Vec3 listenerPos{.x = 0.0f, .y = 0.0f, .z = 0.0f};
  const Vec3 listenerFwd{.x = 0.0f, .y = 0.0f, .z = -1.0f};
  const Vec3 listenerUp{.x = 0.0f, .y = 1.0f, .z = 0.0f};

  constexpr float TOLERANCE = 1e-4f;

  EXPECT_NEAR(
      computeAzimuth({0.0f, 0.0f, -1.0f}, listenerPos, listenerFwd, listenerUp), 0.0f, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({1.0f, 0.0f, 0.0f}, listenerPos, listenerFwd, listenerUp), 90.0f, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({-1.0f, 0.0f, 0.0f}, listenerPos, listenerFwd, listenerUp), -90.0f, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({0.0f, 0.0f, 0.0f}, listenerPos, listenerFwd, listenerUp), 0.0f, TOLERANCE);
  EXPECT_NEAR(
      computeAzimuth({0.0f, 0.0f, 1.0f}, listenerPos, listenerFwd, listenerUp), -180.0f, TOLERANCE);
}

TEST(PannerSpatializationTest, ComputeDistance) {
  EXPECT_FLOAT_EQ(computeDistance({0.0f, 0.0f, 0.0f}, {3.0f, 4.0f, 0.0f}), 5.0f);
  EXPECT_FLOAT_EQ(computeDistance({1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}), 0.0f);
}

TEST(PannerSpatializationTest, ComputeDistanceGain) {
  constexpr double maxDist = 10000.0;

  EXPECT_FLOAT_EQ(computeDistanceGain(DistanceModelType::Inverse, 10.0f, 1.0, maxDist, 1.0), 0.1f);

  EXPECT_FLOAT_EQ(computeDistanceGain(DistanceModelType::Inverse, 10.0f, 0.0, maxDist, 1.0), 0.0f);

  EXPECT_FLOAT_EQ(computeDistanceGain(DistanceModelType::Linear, 5.0f, 1.0, 9.0, 1.0), 0.5f);

  EXPECT_FLOAT_EQ(
      computeDistanceGain(DistanceModelType::Exponential, 2.0f, 1.0, maxDist, 1.0), 0.5f);
}

TEST(PannerSpatializationTest, ComputeConeGain) {
  const Vec3 listenerPos{.x = 0.0f, .y = 0.0f, .z = 0.0f};
  const Vec3 sourcePos{.x = 0.0f, .y = 0.0f, .z = -1.0f};

  constexpr float SILENCE = 0.0f;
  constexpr float FULL_GAIN = 1.0f;

  // 360/360 cone angles act as a bypass (omnidirectional) => fully audible
  const Vec3 orientationRight{.x = 1.0f, .y = 0.0f, .z = 0.0f};
  EXPECT_FLOAT_EQ(
      computeConeGain(sourcePos, listenerPos, orientationRight, 360.0, 360.0, 0.0), FULL_GAIN);

  // Source is facing front, listener in the inner angle => fully audible
  const Vec3 orientationFront{.x = 0.0f, .y = 0.0f, .z = 1.0f};
  EXPECT_FLOAT_EQ(
      computeConeGain(sourcePos, listenerPos, orientationFront, 60.0, 90.0, 0.0), FULL_GAIN);

  // Source is facing back, listener outside the outer angle => silent
  const Vec3 orientationBack{.x = 0.0f, .y = 0.0f, .z = -1.0f};
  EXPECT_FLOAT_EQ(
      computeConeGain(sourcePos, listenerPos, orientationBack, 60.0, 90.0, 0.0), SILENCE);

  // Interpolation gain equal to 0.5. Angle is 37.5 degrees from the listener.
  const float angle = 37.5f * PI / DEG_180;
  Vec3 orientationLerp{.x = std::sin(angle), .y = 0.0f, .z = std::cos(angle)};
  constexpr float TOLERANCE = 1e-5f;
  EXPECT_NEAR(
      computeConeGain(sourcePos, listenerPos, orientationLerp, 60.0, 90.0, 0.0), 0.5f, TOLERANCE);

  const Vec3 orientationZero{.x = 0.0f, .y = 0.0f, .z = 0.0f};
  EXPECT_FLOAT_EQ(
      computeConeGain(sourcePos, listenerPos, orientationZero, 60.0, 90.0, 0.0), FULL_GAIN);
}
