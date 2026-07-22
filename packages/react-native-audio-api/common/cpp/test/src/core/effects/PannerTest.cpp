#include <audioapi/core/AudioListener.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/PannerNode.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class PannerTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;
  std::shared_ptr<AudioListener> listener;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
    listener = std::make_shared<AudioListener>(context);
  }
};

class TestablePannerNode : public PannerNode {
 public:
  TestablePannerNode(const std::shared_ptr<BaseAudioContext> &context, AudioListener *listener)
      : PannerNode(context, listener, PannerOptions()) {}

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }

  using PannerNode::processNode;
};

TEST_F(PannerTest, PannerCanBeCreated) {
  auto panner = std::make_shared<PannerNode>(context, listener.get(), PannerOptions());
  ASSERT_NE(panner, nullptr);
}

TEST_F(PannerTest, MonoSourcePannedByPosition) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());
  panNode.getPositionXParam()->setValue(1.0f);

  auto buffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelMono))[i] = 1.0f;
  }

  panNode.setInputBuffer(buffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();
  const float left = (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[0];
  const float right = (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[0];

  EXPECT_GT(right, left);
  EXPECT_NEAR(left + right, 1.0f, 0.05f);
}

TEST_F(PannerTest, DistanceAttenuatesSignal) {
  static constexpr int FRAMES_TO_PROCESS = 4;
  TestablePannerNode panNode(context, listener.get());
  panNode.getPositionZParam()->setValue(-10.0f);
  panNode.setRefDistance(1.0);
  panNode.setRolloffFactor(1.0);
  panNode.setDistanceModel(DistanceModelType::Inverse);

  auto buffer = std::make_shared<DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannelByType(AudioBuffer::ChannelMono))[i] = 1.0f;
  }

  panNode.setInputBuffer(buffer);
  panNode.processNode(FRAMES_TO_PROCESS);

  auto resultBuffer = panNode.getOutputBuffer();
  const float left = (*resultBuffer->getChannelByType(AudioBuffer::ChannelLeft))[0];
  const float right = (*resultBuffer->getChannelByType(AudioBuffer::ChannelRight))[0];

  EXPECT_LT(left + right, 1.0f);
}

// NOLINTEND
