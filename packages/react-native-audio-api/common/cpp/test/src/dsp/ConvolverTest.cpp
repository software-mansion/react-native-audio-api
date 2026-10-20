#include <audioapi/dsp/Convolver.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/ThreadPool.hpp>
#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <random>
#include <vector>

namespace audioapi {

/// Test-side definition of the friend declared in Convolver.h.
struct ConvolverInspector {
  static size_t tailMisses(const Convolver &convolver) {
    return convolver._tailMisses;
  }
};
} // namespace audioapi

namespace audioapi::test {

namespace {

constexpr size_t kBlockSize = 128;
// Float accumulation over thousands of taps drifts by a few ULPs of the result.
constexpr float kAbsTolerance = 1e-4f;
constexpr float kRelTolerance = 1e-5f;

std::vector<float> randomSamples(size_t count, unsigned seed) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::vector<float> samples(count);
  for (auto &sample : samples) {
    sample = dist(rng);
  }
  return samples;
}

/// Plain time-domain convolution, the ground truth every FFT path must match.
std::vector<float> directConvolution(
    const std::vector<float> &input,
    const std::vector<float> &ir) {
  std::vector<float> out(input.size(), 0.0f);
  for (size_t n = 0; n < input.size(); ++n) {
    for (size_t i = 0; i < ir.size() && i <= n; ++i) {
      out[n] += input[n - i] * ir[i];
    }
  }
  return out;
}

struct Fixture {
  std::vector<float> ir;
  std::vector<float> input;
  std::vector<float> expected;
  size_t blocks;

  Fixture(size_t irLength, size_t blockCount, unsigned seed)
      : ir(randomSamples(irLength, seed)),
        input(randomSamples(blockCount * kBlockSize, seed + 1)),
        expected(directConvolution(input, ir)),
        blocks(blockCount) {}

  std::shared_ptr<Convolver> makeConvolver() const {
    AudioArray irArray(ir.size());
    for (size_t i = 0; i < ir.size(); ++i) {
      irArray[i] = ir[i];
    }
    auto convolver = std::make_shared<Convolver>();
    EXPECT_TRUE(convolver->init(kBlockSize, irArray, ir.size()));
    return convolver;
  }

  void feedBlock(size_t block, DSPAudioArray &in) const {
    for (size_t i = 0; i < kBlockSize; ++i) {
      in[i] = input[block * kBlockSize + i];
    }
  }

  void expectBlock(size_t block, const DSPAudioArray &out) const {
    for (size_t i = 0; i < kBlockSize; ++i) {
      const float want = expected[block * kBlockSize + i];
      ASSERT_NEAR(out[i], want, kAbsTolerance + kRelTolerance * std::abs(want))
          << "block " << block << " sample " << i;
    }
  }
};

} // namespace

TEST(ConvolverTest, InlineTailMatchesDirectConvolution) {
  Fixture fx(/*irLength=*/2000, /*blockCount=*/40, /*seed=*/1);
  auto convolver = fx.makeConvolver();
  ASSERT_TRUE(convolver->hasTail());
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
  }
  // Nothing precomputed the tails, so every block past the silent prefix fell back.
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), fx.blocks - Convolver::kHeadSegments);
}

TEST(ConvolverTest, ShortIrHasNoTailAndMatchesDirectConvolution) {
  Fixture fx(/*irLength=*/300, /*blockCount=*/12, /*seed=*/2);
  auto convolver = fx.makeConvolver();
  EXPECT_FALSE(convolver->hasTail());
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, PrecomputedTailMatchesDirectConvolution) {
  Fixture fx(/*irLength=*/3000, /*blockCount=*/60, /*seed=*/3);
  auto convolver = fx.makeConvolver();
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
    // A worker that always makes its deadline.
    convolver->computeTail(convolver->nextTailBlock());
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, LateTailIsDroppedAndOutputStaysCorrect) {
  Fixture fx(/*irLength=*/2000, /*blockCount=*/40, /*seed=*/4);
  auto convolver = fx.makeConvolver();
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < 20; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
  }
  // The worker for block 10 shows up long after the audio thread rendered it.
  convolver->computeTail(10);
  const size_t missesSoFar = ConvolverInspector::tailMisses(*convolver);

  for (size_t block = 20; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), missesSoFar + (fx.blocks - 20));
}

TEST(ConvolverTest, TailsScheduledOnThreadPoolMatchDirectConvolution) {
  Fixture fx(/*irLength=*/4000, /*blockCount=*/300, /*seed=*/5);
  auto convolver = fx.makeConvolver();
  ConvolverThreadPool pool(2);
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
    pool.schedule([convolver, tail = convolver->nextTailBlock()] { convolver->computeTail(tail); });
  }
  pool.wait();
  // Hits and misses depend on scheduling; the output must not.
  EXPECT_LT(ConvolverInspector::tailMisses(*convolver), fx.blocks);
}

} // namespace audioapi::test
