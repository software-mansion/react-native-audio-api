#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr int kQuantum = 128;
constexpr int kSampleRate = 44100;
constexpr float kInputLeft = 0.5f;
constexpr float kInputRight = 0.25f;
// One-sample impulse responses, so each convolver is a plain gain.
constexpr float kResponseGains[4] = {1.0f, 0.5f, 0.25f, 0.75f};
// Chrome: L = IR0*L + IR2*R, R = IR1*L + IR3*R.
constexpr float kExpectedLeft = kResponseGains[0] * kInputLeft + kResponseGains[2] * kInputRight;
constexpr float kExpectedRight = kResponseGains[1] * kInputLeft + kResponseGains[3] * kInputRight;
// A mono bus feeds kInputLeft to all four responses and averages the stereo pair.
constexpr float kExpectedMono = 0.5f * kInputLeft *
    (kResponseGains[0] + kResponseGains[1] + kResponseGains[2] + kResponseGains[3]);

// Chrome's Reverb::CalculateNormalizationScale ends with a 0.5 true-stereo compensation.
float expectedNormalizationScale() {
  float power = 0.0f;
  for (float gain : kResponseGains) {
    power += gain * gain;
  }
  power = std::sqrt(power / (4 * kQuantum));
  return 1.0f / power * std::pow(10.0f, GAIN_CALIBRATION * 0.05f) * 0.5f;
}

class ConvolverHarness : public ConvolverNode {
 public:
  using ConvolverNode::ConvolverNode;

  void runQuantum(const std::vector<const DSPAudioBuffer *> &inputs) {
    this->processInputs(inputs, kQuantum);
  }
};

class ConvolverTrueStereoTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * kSampleRate, kSampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }

  // Renders a constant input through a 4-channel response and returns {first, last} bus channel.
  std::pair<float, float> render(bool normalize, int busChannels = 2) {
    auto response = std::make_shared<AudioBuffer>(kQuantum, 4, kSampleRate);
    response->zero();
    for (int channel = 0; channel < 4; ++channel) {
      (*response->getChannel(channel))[0] = kResponseGains[channel];
    }

    std::vector<std::shared_ptr<Convolver>> convolvers;
    for (size_t channel = 0; channel < 4; ++channel) {
      AudioArray channelData(*response->getChannel(channel));
      convolvers.push_back(std::make_shared<Convolver>());
      EXPECT_TRUE(convolvers.back()->init(kQuantum, channelData, response->getSize()));
    }

    ConvolverHarness convolver(context, ConvolverOptions());
    const float scale = normalize ? convolver.calculateNormalizationScale(response) : 1.0f;
    convolver.setOutputBuffer(std::make_shared<DSPAudioBuffer>(kQuantum, busChannels, kSampleRate));

    convolver.setBuffer(
        response,
        std::move(convolvers),
        /*threadPool=*/nullptr,
        std::make_shared<DSPAudioBuffer>(kQuantum, 4, kSampleRate),
        scale);

    auto input = std::make_shared<DSPAudioBuffer>(kQuantum, busChannels, kSampleRate);
    for (int quantum = 0; quantum < 4; ++quantum) {
      for (size_t i = 0; i < kQuantum; ++i) {
        (*input->getChannel(0))[i] = kInputLeft;
        (*input->getChannel(busChannels - 1))[i] = busChannels == 1 ? kInputLeft : kInputRight;
      }
      convolver.runQuantum({input.get()});
    }

    const auto *output = convolver.getOutput();
    EXPECT_EQ(output->getNumberOfChannels(), static_cast<size_t>(busChannels));
    return {
        (*output->getChannel(0))[kQuantum / 2],
        (*output->getChannel(busChannels - 1))[kQuantum / 2]};
  }
};

} // namespace

TEST_F(ConvolverTrueStereoTest, FourChannelResponseHasUnityGainWithoutNormalization) {
  const auto [left, right] = render(/*normalize=*/false);

  EXPECT_NEAR(left, kExpectedLeft, 1e-4f);
  EXPECT_NEAR(right, kExpectedRight, 1e-4f);
}

TEST_F(ConvolverTrueStereoTest, FourChannelResponseIsHalvedByNormalization) {
  const auto [left, right] = render(/*normalize=*/true);

  EXPECT_NEAR(left, kExpectedLeft * expectedNormalizationScale(), 1e-6f);
  EXPECT_NEAR(right, kExpectedRight * expectedNormalizationScale(), 1e-6f);
}

TEST_F(ConvolverTrueStereoTest, MonoBusAveragesTheStereoPair) {
  const auto [mono, _] = render(/*normalize=*/false, /*busChannels=*/1);

  EXPECT_NEAR(mono, kExpectedMono, 1e-4f);
}

TEST_F(ConvolverTrueStereoTest, MonoBusIsHalvedByNormalization) {
  const auto [mono, _] = render(/*normalize=*/true, /*busChannels=*/1);

  EXPECT_NEAR(mono, kExpectedMono * expectedNormalizationScale(), 1e-6f);
}

// NOLINTEND
