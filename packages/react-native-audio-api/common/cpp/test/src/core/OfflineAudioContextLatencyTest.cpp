#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

TEST(OfflineAudioContextLatencyTest, BaseLatencyIsZero) {
  auto eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
  auto context =
      std::make_shared<OfflineAudioContext>(2, 44100, 44100.0f, eventRegistry, RuntimeRegistry{});

  EXPECT_DOUBLE_EQ(context->getBaseLatency(), 0.0);
}
