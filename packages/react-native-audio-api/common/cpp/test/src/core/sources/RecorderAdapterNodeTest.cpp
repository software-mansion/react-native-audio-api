#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

namespace audioapi::test {

namespace {

const StreamFormat MONO_44K{
    .layout = {.sampleRate = 44100.0f, .channelCount = 1},
    .maxFramesPerBuffer = 256,
};

std::shared_ptr<OfflineAudioContext> makeContext() {
  return std::make_shared<OfflineAudioContext>(
      2, 128, 44100.0f, std::make_shared<MockAudioEventHandlerRegistry>());
}

} // namespace

TEST(RecorderAdapterNodeTest, ItsConsumerSizesTheNodeForTheRecordersFormat) {
  auto context = makeContext();
  RecorderAdapterNode adapter(context);

  ASSERT_TRUE(adapter.consumer()->prepare(MONO_44K).is_ok());

  EXPECT_EQ(adapter.consumer()->preparedChannelCount(), 1U);
  EXPECT_EQ(adapter.getOutputChannelNumber(), 1U);
  adapter.consumer()->close();
  EXPECT_EQ(adapter.consumer()->preparedChannelCount(), 0U);
}

TEST(RecorderAdapterNodeTest, ItsConsumerOutlivesTheNodesContext) {
  auto context = makeContext();
  auto consumer = RecorderAdapterNode(context).consumer();

  context.reset();

  ASSERT_TRUE(consumer->prepare(MONO_44K).is_ok());
  EXPECT_EQ(consumer->preparedChannelCount(), 1U);
  consumer->close();
}

} // namespace audioapi::test
