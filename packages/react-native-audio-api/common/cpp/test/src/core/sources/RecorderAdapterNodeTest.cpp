#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

namespace audioapi::test {

TEST(RecorderAdapterNodeTest, TrivialConstructAndInit) {
  auto registry = std::make_shared<MockAudioEventHandlerRegistry>();
  auto context = std::make_shared<OfflineAudioContext>(2, 128, 44100.0f, registry);
  RecorderAdapterNode adapter(context);
  adapter.init(256, 1, 44100.0f);
  adapter.adapterCleanup();
}

} // namespace audioapi::test
