#include <gtest/gtest.h>

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

#include "../../MockAudioEventHandlerRegistry.h"

using namespace audioapi;

namespace {

constexpr float kSampleRate = 48000.0f;

class ConvolverThreadPoolTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;

  std::shared_ptr<OfflineAudioContext> makeContext() {
    return std::make_shared<OfflineAudioContext>(2, kSampleRate, kSampleRate, eventRegistry);
  }

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
  }
};

TEST_F(ConvolverThreadPoolTest, ContextHandsOutOneSharedPool) {
  auto context = makeContext();

  auto first = context->getConvolverThreadPool();
  auto second = context->getConvolverThreadPool();

  ASSERT_NE(first, nullptr);
  EXPECT_EQ(first, second) << "Repeated buffer assignments must reuse the context pool";
}

TEST_F(ConvolverThreadPoolTest, PoolIsPerContext) {
  auto contextA = makeContext();
  auto contextB = makeContext();

  EXPECT_NE(contextA->getConvolverThreadPool(), contextB->getConvolverThreadPool());
}

TEST_F(ConvolverThreadPoolTest, ReassigningBufferKeepsTheSharedPoolAlive) {
  auto context = makeContext();
  auto destination = std::make_shared<AudioDestinationNode>(context);
  context->initialize(destination.get());

  auto pool = context->getConvolverThreadPool();
  auto node = std::make_shared<ConvolverNode>(context, ConvolverOptions());

  auto firstIr = std::make_shared<AudioBuffer>(1024, 1, kSampleRate);
  auto secondIr = std::make_shared<AudioBuffer>(512, 1, kSampleRate);
  node->setBuffer(firstIr, {}, pool, nullptr, nullptr, 1.0f);
  node->setBuffer(secondIr, {}, pool, nullptr, nullptr, 1.0f);

  // Context + node + this test each hold one reference; the old assignment
  // must not have handed the shared pool to the disposer.
  EXPECT_EQ(pool.use_count(), 3);
  EXPECT_EQ(pool, context->getConvolverThreadPool());
}

} // namespace
