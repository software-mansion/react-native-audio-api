#include <test/BiquadFilterTest.h>

namespace audioapi {
TEST_F(BiquadFilterTest, TestLowpassCoefficients) {
  auto filterNode = std::make_shared<audioapi::BiquadFilterNode>(context.get());

  // Test private method directly
  filterNode->setLowpassCoefficients(1000.0f, 1.0f);
}
} // namespace audioapi