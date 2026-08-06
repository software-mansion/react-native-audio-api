#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/types/NodeOptions.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

namespace audioapi::test {

TEST(AnalyserNodeTest, TrivialConstruct) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  constexpr size_t LENGTH = 128;
  constexpr float SAMPLE_RATE = 44100.0f;
  auto context = std::make_shared<OfflineAudioContext>(2, LENGTH, SAMPLE_RATE, registry);
  AnalyserNode analyser(context, AnalyserOptions{});
  EXPECT_EQ(analyser.getFFTSize(), AnalyserOptions::kDefaultFftSize);
}

} // namespace audioapi::test
