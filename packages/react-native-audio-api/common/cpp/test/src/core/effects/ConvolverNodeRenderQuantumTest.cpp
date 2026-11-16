#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr int kSampleRate = 44100;
constexpr int kRenderQuantum = 441;
constexpr size_t kConvolverBlock = 512;
constexpr size_t kIrLength = 3000;
constexpr size_t kSignalLength = 8 * kRenderQuantum + 123;
constexpr float kTolerance = 1e-4f;

class ConvolverNodeHarness : public ConvolverNode {
 public:
  using ConvolverNode::ConvolverNode;

  /// Feeds one render call straight into processNode, the way the graph does after mixing
  /// inputs into `audioBuffer_`.
  void render(const AudioBuffer &input, size_t offset, int frames, AudioBuffer &output) {
    audioBuffer_->zero();
    for (size_t ch = 0; ch < audioBuffer_->getNumberOfChannels(); ++ch) {
      audioBuffer_->getChannel(ch)->copy(*input.getChannel(ch), offset, 0, frames);
    }
    processNode(frames);
    for (size_t ch = 0; ch < output.getNumberOfChannels(); ++ch) {
      output.getChannel(ch)->copy(*audioBuffer_->getChannel(ch), 0, offset, frames);
    }
  }

  [[nodiscard]] size_t outputCapacity() const {
    return audioBuffer_->getSize();
  }
};

std::shared_ptr<AudioBuffer> randomBuffer(size_t length, int channels, unsigned seed, float gain) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  auto buffer = std::make_shared<AudioBuffer>(length, channels, kSampleRate);
  for (int ch = 0; ch < channels; ++ch) {
    for (size_t i = 0; i < length; ++i) {
      (*buffer->getChannel(ch))[i] = gain * dist(rng);
    }
  }
  return buffer;
}

float directConvolutionAt(const AudioArray &input, const AudioArray &ir, size_t n) {
  double acc = 0.0;
  const size_t taps = std::min(ir.getSize(), n + 1);
  for (size_t k = 0; k < taps; ++k) {
    acc += static_cast<double>(ir[k]) * input[n - k];
  }
  return static_cast<float>(acc);
}

class ConvolverNodeRenderQuantumTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * kSampleRate, kSampleRate, eventRegistry, kRenderQuantum);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }

  /// Mirrors ConvolverNodeHostObject::setBuffer for a stereo IR, without normalization.
  void loadImpulseResponse(
      ConvolverNodeHarness &node,
      const std::shared_ptr<AudioBuffer> &ir,
      size_t blockSize = kConvolverBlock) {
    std::vector<std::unique_ptr<Convolver>> convolvers;
    for (size_t ch = 0; ch < ir->getNumberOfChannels(); ++ch) {
      AudioArray channel(*ir->getChannel(ch));
      convolvers.push_back(std::make_unique<Convolver>());
      convolvers.back()->init(blockSize, channel, ir->getSize());
    }
    auto intermediate = std::make_shared<DSPAudioBuffer>(
        context->getRenderQuantumSize(), convolvers.size(), context->getSampleRate());
    node.setBuffer(
        ir, std::move(convolvers), context->getConvolverThreadPool(), intermediate, 1.0f);
  }

  void expectStereoConvolution(
      const AudioBuffer &input,
      const AudioBuffer &ir,
      const AudioBuffer &output,
      size_t length = kSignalLength) {
    for (int ch = 0; ch < 2; ++ch) {
      float maxError = 0.0f;
      for (size_t n = 0; n < length; ++n) {
        const float expected = directConvolutionAt(*input.getChannel(ch), *ir.getChannel(ch), n);
        maxError = std::max(maxError, std::fabs((*output.getChannel(ch))[n] - expected));
      }
      EXPECT_LT(maxError, kTolerance) << "channel " << ch;
    }
  }
};

} // namespace

TEST_F(ConvolverNodeRenderQuantumTest, ContextSizesNodeBuffersToItsRenderQuantum) {
  ConvolverNodeHarness node(context, ConvolverOptions());

  EXPECT_EQ(context->getRenderQuantumSize(), kRenderQuantum);
  EXPECT_EQ(node.outputCapacity(), static_cast<size_t>(kRenderQuantum));
}

TEST_F(ConvolverNodeRenderQuantumTest, ConvolvesNonPowerOfTwoQuanta) {
  ConvolverNodeHarness node(context, ConvolverOptions());
  const auto ir = randomBuffer(kIrLength, 2, 11, 1.0f / std::sqrt(static_cast<float>(kIrLength)));
  loadImpulseResponse(node, ir);

  const auto input = randomBuffer(kSignalLength, 2, 12, 1.0f);
  AudioBuffer output(kSignalLength, 2, kSampleRate);

  for (size_t offset = 0; offset < kSignalLength; offset += kRenderQuantum) {
    const int frames = static_cast<int>(std::min<size_t>(kRenderQuantum, kSignalLength - offset));
    node.render(*input, offset, frames, output);
  }

  expectStereoConvolution(*input, *ir, output);
}

TEST_F(ConvolverNodeRenderQuantumTest, ConvolvesQuantaThatVaryEveryCall) {
  ConvolverNodeHarness node(context, ConvolverOptions());
  const auto ir = randomBuffer(kIrLength, 2, 21, 1.0f / std::sqrt(static_cast<float>(kIrLength)));
  loadImpulseResponse(node, ir);

  const auto input = randomBuffer(kSignalLength, 2, 22, 1.0f);
  AudioBuffer output(kSignalLength, 2, kSampleRate);

  // The system may hand out any callback size up to the render quantum.
  std::mt19937 rng(23);
  std::uniform_int_distribution<int> frameDist(1, kRenderQuantum);
  for (size_t offset = 0; offset < kSignalLength;) {
    const int frames = std::min(frameDist(rng), static_cast<int>(kSignalLength - offset));
    node.render(*input, offset, frames, output);
    offset += frames;
  }

  expectStereoConvolution(*input, *ir, output);
}

TEST_F(ConvolverNodeRenderQuantumTest, ConvolvesTheLargestQuantumTheSpecAllows) {
  // Web Audio API §2.5: up to 6 seconds of frames per quantum. At the lowest supported sample
  // rate that is 18000 frames, longer than the largest convolver block, so every call spans
  // several blocks.
  constexpr int kLowSampleRate = 3000;
  constexpr int kLargestQuantum = 6 * kLowSampleRate;
  constexpr size_t kLargestBlock = 8192;
  constexpr size_t kLength = 2 * kLargestQuantum + 500;
  context = std::make_shared<OfflineAudioContext>(
      2, kLength, kLowSampleRate, eventRegistry, kLargestQuantum);
  destination = std::make_shared<AudioDestinationNode>(context);
  context->initialize(destination.get());

  ConvolverNodeHarness node(context, ConvolverOptions());
  ASSERT_EQ(node.outputCapacity(), static_cast<size_t>(kLargestQuantum));
  const auto ir = randomBuffer(1000, 2, 31, 1.0f / std::sqrt(1000.0f));
  loadImpulseResponse(node, ir, kLargestBlock);

  const auto input = randomBuffer(kLength, 2, 32, 1.0f);
  AudioBuffer output(kLength, 2, kLowSampleRate);
  for (size_t offset = 0; offset < kLength; offset += kLargestQuantum) {
    const int frames = static_cast<int>(std::min<size_t>(kLargestQuantum, kLength - offset));
    node.render(*input, offset, frames, output);
  }

  expectStereoConvolution(*input, *ir, output, kLength);
}

// NOLINTEND
