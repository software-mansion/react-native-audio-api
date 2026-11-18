#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

namespace audioapi::test {

// TODO: replace this testcase with more meaningful ones.
TEST(RecorderAdapterNodeTest, TrivialConstructAndOpen) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  auto context = std::make_shared<OfflineAudioContext>(2, 128, 44100.0f, registry);
  RecorderAdapterNode adapter(context);
  const StreamFormat format{
      .layout = {.sampleRate = 44100.0f, .channelCount = 1}, .maxFramesPerBuffer = 256};
  EXPECT_TRUE(adapter.prepare(format).is_ok());
  adapter.close();
}

} // namespace audioapi::test
