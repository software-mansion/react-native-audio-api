#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr int kSampleRate = 24000;
constexpr int kQuantum = RENDER_QUANTUM_SIZE;
constexpr float kPlaybackRate = 1.5f;
constexpr float kOnesThreshold = 0.5f;

constexpr size_t kScheduleUnitFrames = 128;

/// Additive join pull-in on top of t1 + D/pr.
/// Since the WSOLA path stopped feeding start-quantum silence into the analysis
/// queue, no compensation is needed: with priming+drain the correct join is L=0.
/// Override via WSOLA_KL_SECONDS to explore other schedules.
constexpr double kL = 0.0;

double joinCompensationSeconds() {
  if (const char *env = std::getenv("WSOLA_KL_SECONDS")) {
    return std::atof(env);
  }
  return kL;
}

float playbackRateForTest() {
  if (const char *env = std::getenv("WSOLA_RATE")) {
    return static_cast<float>(std::atof(env));
  }
  return kPlaybackRate;
}

size_t sufficientPartFrames(float playbackRate) {
  WsolaTimeStretcher stretcher;
  stretcher.configure(1, static_cast<float>(kSampleRate));
  const size_t minIn =
      std::max(stretcher.getRequiredInputFrames(), stretcher.getMinInputFramesToRun());
  const size_t needed = static_cast<size_t>(
      std::ceil(5.0 * static_cast<double>(minIn) * static_cast<double>(playbackRate)));
  const size_t units = (needed + kScheduleUnitFrames - 1) / kScheduleUnitFrames;
  return units * kScheduleUnitFrames;
}

std::shared_ptr<AudioBuffer> makeOnesBuffer(size_t frames) {
  auto buffer = std::make_shared<AudioBuffer>(frames, 1, static_cast<float>(kSampleRate));
  auto *ch = buffer->getChannel(0)->begin();
  for (size_t i = 0; i < frames; ++i) {
    ch[i] = 1.0f;
  }
  return buffer;
}

std::shared_ptr<AudioBuffer> makeSineBuffer(size_t frames, double phase0, float freqHz) {
  auto buffer = std::make_shared<AudioBuffer>(frames, 1, static_cast<float>(kSampleRate));
  auto *ch = buffer->getChannel(0)->begin();
  const double omega = 2.0 * PI * static_cast<double>(freqHz) / static_cast<double>(kSampleRate);
  for (size_t i = 0; i < frames; ++i) {
    ch[i] = static_cast<float>(std::sin(phase0 + omega * static_cast<double>(i)));
  }
  return buffer;
}

constexpr size_t kEnergyWindow = 64;
constexpr float kEnergyThreshold = 0.05f;

struct EnergyRun {
  size_t start = 0;
  size_t length = 0;
};

float windowRms(const std::vector<float> &samples, size_t start, size_t len) {
  if (len == 0 || start >= samples.size()) {
    return 0.0f;
  }
  const size_t end = std::min(start + len, samples.size());
  double sumSq = 0.0;
  for (size_t i = start; i < end; ++i) {
    sumSq += static_cast<double>(samples[i]) * static_cast<double>(samples[i]);
  }
  return static_cast<float>(std::sqrt(sumSq / static_cast<double>(end - start)));
}

size_t countEnergyRuns(const std::vector<float> &samples) {
  size_t runs = 0;
  bool inRun = false;
  for (size_t i = 0; i + kEnergyWindow <= samples.size(); i += kEnergyWindow) {
    const bool active = windowRms(samples, i, kEnergyWindow) >= kEnergyThreshold;
    if (active && !inRun) {
      ++runs;
      inRun = true;
    } else if (!active) {
      inRun = false;
    }
  }
  return runs;
}

EnergyRun longestEnergyRun(const std::vector<float> &samples) {
  EnergyRun best;
  size_t i = 0;
  while (i + kEnergyWindow <= samples.size()) {
    if (windowRms(samples, i, kEnergyWindow) < kEnergyThreshold) {
      i += kEnergyWindow;
      continue;
    }
    const size_t start = i;
    while (i + kEnergyWindow <= samples.size() &&
           windowRms(samples, i, kEnergyWindow) >= kEnergyThreshold) {
      i += kEnergyWindow;
    }
    const size_t len = i - start;
    if (len > best.length) {
      best.start = start;
      best.length = len;
    }
  }
  return best;
}

struct OnesRun {
  size_t start = 0;
  size_t length = 0;
};

/// Calculates the longest continuous sequence of ones (values ≥ kOnesThreshold)
OnesRun longestOnesRun(const std::vector<float> &samples) {
  OnesRun best;
  size_t i = 0;
  while (i < samples.size()) {
    if (samples[i] < kOnesThreshold) {
      ++i;
      continue;
    }
    const size_t start = i;
    while (i < samples.size() && samples[i] >= kOnesThreshold) {
      ++i;
    }
    const size_t len = i - start;
    if (len > best.length) {
      best.start = start;
      best.length = len;
    }
  }
  return best;
}

size_t countOnesRuns(const std::vector<float> &samples) {
  size_t runs = 0;
  size_t i = 0;
  while (i < samples.size()) {
    if (samples[i] < kOnesThreshold) {
      ++i;
      continue;
    }
    ++runs;
    while (i < samples.size() && samples[i] >= kOnesThreshold) {
      ++i;
    }
  }
  return runs;
}

class TestableAudioBufferSourceNode : public AudioBufferSourceNode {
 public:
  explicit TestableAudioBufferSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioBufferSourceOptions &options)
      : AudioBufferSourceNode(context, options) {}

  using AudioBufferSourceNode::initStretch;
  using AudioBufferSourceNode::setBuffer;
};

template <typename NodeT>
NodeT *nodeOf(utils::graph::HostGraph::Node *hostNode) {
  return static_cast<NodeT *>(hostNode->handle->audioNode->asAudioNode());
}

} // namespace

/// Join schedule:
///   b1.start(t1)
///   b2.start(t1 + D / pr + L)
/// where D = x / sr (buffer duration), pr = playbackRate.
/// L is an additive join pull-in; the correct value is 0 (see kL).
/// Pass when output is: leading zeros (WSOLA latency), then ONE contiguous ones
/// run (no internal gap). A gap ⇒ L is wrong.
TEST(WsolaScheduleJoinLatencyTest, ContiguousOnesAfterJoin) {
  const double L = joinCompensationSeconds();
  const float pr = playbackRateForTest();
  const size_t x = sufficientPartFrames(pr);
  // Ideal wall-clock ones from two abutted parts (OLA soft edges ⇒ allow < 100%).
  const size_t idealOnes =
      static_cast<size_t>(std::lround(2.0 * static_cast<double>(x) / static_cast<double>(pr)));
  const size_t minOnes = (idealOnes * 90) / 100;
  const size_t captureFrames = idealOnes + static_cast<size_t>(kSampleRate / 2); // 0.5s margin

  auto eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
  const size_t safetyMarginSize = 2;

  auto context = std::make_shared<OfflineAudioContext>(
      1, static_cast<int>(captureFrames * safetyMarginSize), kSampleRate, eventRegistry);
  auto destination = std::make_unique<AudioDestinationNode>(context);
  context->initialize(destination.get());
  auto *destinationHostNode = context->getGraph()->addNode(std::move(destination));

  BaseAudioBufferSourceOptions baseOptions;
  baseOptions.pitchCorrection = true;
  baseOptions.playbackRate = pr;
  baseOptions.detune = 0.0f;
  AudioBufferSourceOptions options(baseOptions);

  auto makeSource = [&]() {
    auto node = std::make_unique<TestableAudioBufferSourceNode>(context, options);
    auto playbackRateBuffer = std::make_shared<DSPAudioBuffer>(
        WsolaTimeStretcher::scratchBufferFrames(static_cast<float>(kSampleRate)),
        1,
        static_cast<float>(kSampleRate));
    node->initStretch(1, static_cast<float>(kSampleRate), playbackRateBuffer);
    auto outBuf = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
    node->setBuffer(makeOnesBuffer(x), outBuf);
    auto *hostNode = context->getGraph()->addNode(std::move(node));
    EXPECT_TRUE(context->getGraph()->addEdge(hostNode, destinationHostNode).is_ok());
    return nodeOf<TestableAudioBufferSourceNode>(hostNode);
  };

  auto *b1 = makeSource();
  auto *b2 = makeSource();

  constexpr double t1 = 0.0;
  const double D = static_cast<double>(x) / static_cast<double>(kSampleRate);
  const double joinAt = t1 + D / static_cast<double>(pr) + L;
  b1->start(t1, 0.0);
  b2->start(joinAt, 0.0);

  std::vector<float> output;
  output.reserve(captureFrames);
  auto quantum = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
  while (output.size() < captureFrames) {
    quantum->zero();
    context->processGraph(quantum.get(), kQuantum); // G.render(128)
    const float *ch = quantum->getChannel(0)->begin();
    for (int i = 0; i < kQuantum; ++i) {
      output.push_back(ch[i]);
    }
  }

  const OnesRun run = longestOnesRun(output);
  const size_t runs = countOnesRuns(output);

  {
    std::cout << "[L search] L=" << L << "s rate=" << pr << " x=" << x << " D=" << D
              << "s joinAt=" << joinAt << "s runs=" << runs << " ideal=" << idealOnes
              << " min=" << minOnes << std::endl;
    size_t i = 0;
    size_t prevEnd = 0;
    bool first = true;
    while (i < output.size()) {
      if (output[i] < kOnesThreshold) {
        ++i;
        continue;
      }
      const size_t start = i;
      while (i < output.size() && output[i] >= kOnesThreshold) {
        ++i;
      }
      std::cout << "[L search]   run [" << start << ", " << i << ") len=" << (i - start);
      if (!first) {
        std::cout << " gapBefore=" << (start - prevEnd);
      }
      std::cout << std::endl;
      prevEnd = i;
      first = false;
    }
  }

  EXPECT_EQ(runs, 1u) << "Gap in ones ⇒ L is wrong. L=" << L << "s onesStart=" << run.start
                      << " onesLen=" << run.length;
  // Primed + COLA-seeded synthesis emits from the scheduled start; allow only
  // sub-quantum jitter before the ones run.
  EXPECT_LE(run.start, static_cast<size_t>(kQuantum))
      << "Ones should begin near the scheduled start (not after WSOLA latency). start="
      << run.start;
  EXPECT_GE(run.length, minOnes) << "Contiguous ones too short (need ≥90% of 2*x/rate). L=" << L
                                 << "s onesLen=" << run.length << " min=" << minOnes
                                 << " ideal=" << idealOnes;
}

/// Same schedule as ContiguousOnesAfterJoin (t1 + D/pr + L), but with a
/// phase-continuous sine split. Pass metric: one contiguous energy run.
TEST(WsolaScheduleJoinLatencyTest, ContiguousSineEnergyAfterJoin) {
  const double L = joinCompensationSeconds();
  const float pr = playbackRateForTest();
  const size_t x = sufficientPartFrames(pr);
  const size_t idealActive =
      static_cast<size_t>(std::lround(2.0 * static_cast<double>(x) / static_cast<double>(pr)));
  const size_t minActive = (idealActive * 85) / 100;
  const size_t captureFrames = idealActive + static_cast<size_t>(kSampleRate / 2);

  auto eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
  auto context = std::make_shared<OfflineAudioContext>(
      1, static_cast<int>(captureFrames * 2), kSampleRate, eventRegistry);
  auto destination = std::make_unique<AudioDestinationNode>(context);
  context->initialize(destination.get());
  auto *destinationHostNode = context->getGraph()->addNode(std::move(destination));

  BaseAudioBufferSourceOptions baseOptions;
  baseOptions.pitchCorrection = true;
  baseOptions.playbackRate = pr;
  baseOptions.detune = 0.0f;
  AudioBufferSourceOptions options(baseOptions);

  constexpr float kSineHz = 440.0f;
  const double omega = 2.0 * PI * static_cast<double>(kSineHz) / static_cast<double>(kSampleRate);
  auto part1 = makeSineBuffer(x, 0.0, kSineHz);
  auto part2 = makeSineBuffer(x, omega * static_cast<double>(x), kSineHz);

  auto makeSource = [&](const std::shared_ptr<AudioBuffer> &buf) {
    auto node = std::make_unique<TestableAudioBufferSourceNode>(context, options);
    auto playbackRateBuffer = std::make_shared<DSPAudioBuffer>(
        WsolaTimeStretcher::scratchBufferFrames(static_cast<float>(kSampleRate)),
        1,
        static_cast<float>(kSampleRate));
    node->initStretch(1, static_cast<float>(kSampleRate), playbackRateBuffer);
    auto outBuf = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
    node->setBuffer(buf, outBuf);
    auto *hostNode = context->getGraph()->addNode(std::move(node));
    EXPECT_TRUE(context->getGraph()->addEdge(hostNode, destinationHostNode).is_ok());
    return nodeOf<TestableAudioBufferSourceNode>(hostNode);
  };

  auto *b1 = makeSource(part1);
  auto *b2 = makeSource(part2);

  constexpr double t1 = 0.0;
  const double D = static_cast<double>(x) / static_cast<double>(kSampleRate);
  const double joinAt = t1 + D / static_cast<double>(pr) + L;
  b1->start(t1, 0.0);
  b2->start(joinAt, 0.0);

  std::vector<float> output;
  output.reserve(captureFrames);
  auto quantum = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
  while (output.size() < captureFrames) {
    quantum->zero();
    context->processGraph(quantum.get(), kQuantum);
    const float *ch = quantum->getChannel(0)->begin();
    for (int i = 0; i < kQuantum; ++i) {
      output.push_back(ch[i]);
    }
  }

  const EnergyRun run = longestEnergyRun(output);
  const size_t runs = countEnergyRuns(output);

  {
    std::cout << "[L search sine] L=" << L << "s rate=" << pr << " x=" << x << " D=" << D
              << "s joinAt=" << joinAt << "s runs=" << runs << " ideal=" << idealActive
              << " min=" << minActive << " longest=" << run.length << " start=" << run.start
              << std::endl;
    size_t i = 0;
    size_t prevEnd = 0;
    bool first = true;
    while (i + kEnergyWindow <= output.size()) {
      if (windowRms(output, i, kEnergyWindow) < kEnergyThreshold) {
        i += kEnergyWindow;
        continue;
      }
      const size_t start = i;
      while (i + kEnergyWindow <= output.size() &&
             windowRms(output, i, kEnergyWindow) >= kEnergyThreshold) {
        i += kEnergyWindow;
      }
      std::cout << "[L search sine]   energy [" << start << ", " << i << ") len=" << (i - start);
      if (!first) {
        std::cout << " gapBefore=" << (start - prevEnd);
      }
      std::cout << std::endl;
      prevEnd = i;
      first = false;
    }
  }

  EXPECT_EQ(runs, 1u) << "Energy gap ⇒ L is wrong for sine. L=" << L << "s start=" << run.start
                      << " len=" << run.length;
  EXPECT_LE(run.start, static_cast<size_t>(kQuantum))
      << "Sine energy should begin near the scheduled start. start=" << run.start;
  EXPECT_GE(run.length, minActive) << "Active energy too short. L=" << L << "s len=" << run.length
                                   << " min=" << minActive << " ideal=" << idealActive;
}

// NOLINTEND
