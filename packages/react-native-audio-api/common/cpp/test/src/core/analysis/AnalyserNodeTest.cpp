#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/types/NodeOptions.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

namespace audioapi::test {

TEST(AnalyserNodeTest, TrivialConstruct) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  auto context = std::make_shared<OfflineAudioContext>(2, 128, 44100.0f, registry);
  AnalyserNode analyser(context, AnalyserOptions{});
  EXPECT_EQ(analyser.getFFTSize(), AnalyserOptions::kDefaultFftSize);
}

} // namespace audioapi::test
