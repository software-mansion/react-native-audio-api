#include <test/biquad/BiquadFilter.h>
#include <test/biquad/BiquadFilterTest.h>

namespace audioapi {

TEST_F(BiquadFilterTest, TestLowpassCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setLowpassCoefficients(frequency, Q);
  auto coeffs = calculateLowpassCoefficients(frequency, Q);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestHighpassCoefficients) {
  float frequency = 500.0f;
  float Q = 0.75f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setHighpassCoefficients(frequency, Q);
  auto coeffs = calculateHighpassCoefficients(frequency, Q);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestBandpassCoefficients) {
  float frequency = 22000.0f;
  float Q = -20.4f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setBandpassCoefficients(frequency, Q);
  auto coeffs = calculateBandpassCoefficients(frequency, Q);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestNotchCoefficients) {
  float frequency = 1000.0f;
  float Q = 10.0f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setNotchCoefficients(frequency, Q);
  auto coeffs = calculateNotchCoefficients(frequency, Q);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestAllpassCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setAllpassCoefficients(frequency, Q);
  auto coeffs = calculateAllpassCoefficients(frequency, Q);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestPeakingCoefficients) {
  float frequency = 1000.0f;
  float Q = 1.0f;
  float gainDB = 3.0f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setPeakingCoefficients(frequency, Q, gainDB);
  auto coeffs = calculatePeakingCoefficients(frequency, Q, gainDB);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestLowshelfCoefficients) {
  float frequency = 1000.0f;
  float gainDB = 3.0f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setLowshelfCoefficients(frequency, gainDB);
  auto coeffs = calculateLowshelfCoefficients(frequency, gainDB);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

TEST_F(BiquadFilterTest, TestHighshelfCoefficients) {
  float frequency = 1000.0f;
  float gainDB = 3.0f;
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  filterNode->setHighshelfCoefficients(frequency, gainDB);
  auto coeffs = calculateHighshelfCoefficients(frequency, gainDB);

  EXPECT_FLOAT_EQ(filterNode->b0_, coeffs.b0);
  EXPECT_FLOAT_EQ(filterNode->b1_, coeffs.b1);
  EXPECT_FLOAT_EQ(filterNode->b2_, coeffs.b2);
  EXPECT_FLOAT_EQ(filterNode->a1_, coeffs.a1);
  EXPECT_FLOAT_EQ(filterNode->a2_, coeffs.a2);
}

} // namespace audioapi
