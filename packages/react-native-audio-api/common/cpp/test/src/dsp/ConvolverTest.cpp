#include <audioapi/dsp/Convolver.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/ThreadPool.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <memory>
#include <random>
#include <string>
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
constexpr size_t kTailBlock = Convolver::TAIL_BLOCK_FRAMES;
constexpr size_t kHeadFrames = Convolver::HEAD_FRAMES;
constexpr size_t kBlocksPerTailBlock = kTailBlock / kBlockSize;
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
  const float *inputData = input.data();
  const float *irData = ir.data();
  for (size_t n = 0; n < input.size(); ++n) {
    const size_t taps = std::min(ir.size(), n + 1);
    // Thousands of taps: a float sum would be less exact than the convolver under test.
    double sum = 0.0;
    for (size_t i = 0; i < taps; ++i) {
      sum += static_cast<double>(inputData[n - i]) * irData[i];
    }
    out[n] = static_cast<float>(sum);
  }
  return out;
}

/// Tail blocks the audio thread has started rendering after `blocks` render quanta.
size_t tailBlocksRendered(size_t blocks) {
  const size_t delayBlocks = kHeadFrames / kTailBlock;
  const size_t started = (blocks + kBlocksPerTailBlock - 1) / kBlocksPerTailBlock;
  return started > delayBlocks ? started - delayBlocks : 0;
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

  /// Renders every block with no helper thread and checks it against the ground truth.
  void renderInline(Convolver &convolver) const {
    DSPAudioArray in(kBlockSize), out(kBlockSize);
    for (size_t block = 0; block < blocks; ++block) {
      feedBlock(block, in);
      convolver.process(in, out);
      ASSERT_NO_FATAL_FAILURE(expectBlock(block, out));
    }
  }
};

} // namespace

TEST(ConvolverTest, ShortIrHasNoTailAndMatchesDirectConvolution) {
  Fixture fx(/*irLength=*/300, /*blockCount=*/12, /*seed=*/2);
  auto convolver = fx.makeConvolver();
  EXPECT_FALSE(convolver->hasTail());

  fx.renderInline(*convolver);

  EXPECT_FALSE(convolver->takeCompletedTailBlock().has_value());
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, IrAsLongAsTheHeadHasNoTail) {
  Fixture fx(/*irLength=*/kHeadFrames, /*blockCount=*/4 * kBlocksPerTailBlock, /*seed=*/6);
  auto convolver = fx.makeConvolver();
  EXPECT_FALSE(convolver->hasTail());

  fx.renderInline(*convolver);

  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, InlineTailMatchesDirectConvolutionAcrossIrLengths) {
  const size_t irLengths[] = {
      kHeadFrames + 1,              // a single tail frame
      kHeadFrames + 1000,           // a partial tail partition
      kHeadFrames + kTailBlock,     // exactly one tail partition
      kHeadFrames + kTailBlock + 1, // one frame into the second partition
  };
  for (const size_t irLength : irLengths) {
    SCOPED_TRACE("irLength " + std::to_string(irLength));
    Fixture fx(irLength, /*blockCount=*/6 * kBlocksPerTailBlock, /*seed=*/1);
    auto convolver = fx.makeConvolver();
    ASSERT_TRUE(convolver->hasTail());

    ASSERT_NO_FATAL_FAILURE(fx.renderInline(*convolver));

    // Nothing precomputed the tail, so every tail block fell back.
    EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), tailBlocksRendered(fx.blocks));
  }
}

TEST(ConvolverTest, InlineTailMatchesDirectConvolutionOnceTheDelayLineWraps) {
  // Four tail partitions and enough blocks to go around their delay line more than once.
  Fixture fx(kHeadFrames + 3 * kTailBlock + 17, /*blockCount=*/10 * kBlocksPerTailBlock, 7);
  auto convolver = fx.makeConvolver();

  fx.renderInline(*convolver);

  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), tailBlocksRendered(fx.blocks));
}

TEST(ConvolverTest, UnitImpulseReturnsTheImpulseResponse) {
  const std::vector<float> ir = randomSamples(kHeadFrames + 2 * kTailBlock + 300, /*seed=*/8);
  AudioArray irArray(ir.data(), ir.size());
  Convolver convolver;
  ASSERT_TRUE(convolver.init(kBlockSize, irArray, ir.size()));
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  const size_t blocks = ir.size() / kBlockSize + 2 * kBlocksPerTailBlock;
  for (size_t block = 0; block < blocks; ++block) {
    in.zero();
    if (block == 0) {
      in[0] = 1.0f;
    }
    convolver.process(in, out);
    for (size_t i = 0; i < kBlockSize; ++i) {
      const size_t frame = block * kBlockSize + i;
      const float want = frame < ir.size() ? ir[frame] : 0.0f;
      ASSERT_NEAR(out[i], want, kAbsTolerance) << "frame " << frame;
    }
  }
}

TEST(ConvolverTest, CompletedTailBlockIsHandedOutOncePerTailBlock) {
  Fixture fx(kHeadFrames + 1000, /*blockCount=*/3 * kBlocksPerTailBlock, /*seed=*/9);
  auto convolver = fx.makeConvolver();
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    const auto completed = convolver->takeCompletedTailBlock();
    const bool completesTailBlock = (block + 1) % kBlocksPerTailBlock == 0;
    ASSERT_EQ(completed.has_value(), completesTailBlock) << "block " << block;
    if (completesTailBlock) {
      EXPECT_EQ(*completed, (block + 1) / kBlocksPerTailBlock - 1);
    }
    EXPECT_FALSE(convolver->takeCompletedTailBlock().has_value()) << "block " << block;
  }
}

TEST(ConvolverTest, PrecomputedTailMatchesDirectConvolution) {
  Fixture fx(kHeadFrames + 2 * kTailBlock + 500, /*blockCount=*/8 * kBlocksPerTailBlock, 3);
  auto convolver = fx.makeConvolver();
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
    // A worker that always makes its deadline.
    if (const auto completed = convolver->takeCompletedTailBlock()) {
      convolver->computeTail(*completed);
    }
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, TailIsUsedUpToOneTailBlockAfterItsInputCompletes) {
  struct PendingTail {
    size_t tailBlock;
    size_t completedAt;
  };
  Fixture fx(kHeadFrames + kTailBlock + 500, /*blockCount=*/6 * kBlocksPerTailBlock, 10);

  // The worker runs `delay` render quanta after the tail block's input completed.
  for (size_t delay = 0; delay <= kBlocksPerTailBlock + 1; ++delay) {
    SCOPED_TRACE("delay " + std::to_string(delay));
    auto convolver = fx.makeConvolver();
    DSPAudioArray in(kBlockSize), out(kBlockSize);
    std::deque<PendingTail> pending;

    for (size_t block = 0; block < fx.blocks; ++block) {
      fx.feedBlock(block, in);
      convolver->process(in, out);
      ASSERT_NO_FATAL_FAILURE(fx.expectBlock(block, out));
      if (const auto completed = convolver->takeCompletedTailBlock()) {
        pending.push_back({*completed, block});
      }
      if (!pending.empty() && block - pending.front().completedAt == delay) {
        convolver->computeTail(pending.front().tailBlock);
        pending.pop_front();
      }
    }

    const bool madeTheDeadline = delay <= kBlocksPerTailBlock;
    EXPECT_EQ(
        ConvolverInspector::tailMisses(*convolver),
        madeTheDeadline ? 0u : tailBlocksRendered(fx.blocks));
  }
}

TEST(ConvolverTest, LateTailIsDroppedAndOutputStaysCorrect) {
  Fixture fx(kHeadFrames + kTailBlock + 500, /*blockCount=*/7 * kBlocksPerTailBlock, 4);
  auto convolver = fx.makeConvolver();
  DSPAudioArray in(kBlockSize), out(kBlockSize);
  const size_t blocksBeforeLateTail = 4 * kBlocksPerTailBlock + 3;

  for (size_t block = 0; block < blocksBeforeLateTail; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
  }
  // The worker for tail block 1 shows up after the audio thread rendered it.
  convolver->computeTail(1);

  for (size_t block = blocksBeforeLateTail; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), tailBlocksRendered(fx.blocks));
}

TEST(ConvolverTest, StrayTailRequestsNeverReplaceAPublishedBlock) {
  Fixture fx(kHeadFrames + 2 * kTailBlock + 500, /*blockCount=*/9 * kBlocksPerTailBlock, 11);
  auto convolver = fx.makeConvolver();
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
    const auto completed = convolver->takeCompletedTailBlock();
    if (!completed) {
      continue;
    }
    // Its input is not complete yet: publishing it now would publish the wrong frames.
    convolver->computeTail(*completed + 1);
    convolver->computeTail(*completed);
    // A duplicate, and the previous owner of the same slot.
    convolver->computeTail(*completed);
    if (*completed >= 3) {
      convolver->computeTail(*completed - 3);
    }
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, TailsComputedOnThreadPoolAreUsed) {
  Fixture fx(kHeadFrames + 3 * kTailBlock, /*blockCount=*/12 * kBlocksPerTailBlock, 12);
  auto convolver = fx.makeConvolver();
  ConvolverThreadPool pool(2);
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
    if (const auto completed = convolver->takeCompletedTailBlock()) {
      pool.schedule([convolver, tail = *completed] { convolver->computeTail(tail); });
      // Stands in for the real time a tail block gives the worker.
      pool.wait();
    }
  }
  EXPECT_EQ(ConvolverInspector::tailMisses(*convolver), 0u);
}

TEST(ConvolverTest, TailsRacingTheAudioThreadMatchDirectConvolution) {
  Fixture fx(kHeadFrames + 3 * kTailBlock, /*blockCount=*/24 * kBlocksPerTailBlock, 5);
  auto convolver = fx.makeConvolver();
  ConvolverThreadPool pool(2);
  DSPAudioArray in(kBlockSize), out(kBlockSize);

  for (size_t block = 0; block < fx.blocks; ++block) {
    fx.feedBlock(block, in);
    convolver->process(in, out);
    fx.expectBlock(block, out);
    if (const auto completed = convolver->takeCompletedTailBlock()) {
      pool.schedule([convolver, tail = *completed] { convolver->computeTail(tail); });
    }
  }
  pool.wait();
  // Hits and misses depend on scheduling; the output must not.
  EXPECT_LE(ConvolverInspector::tailMisses(*convolver), tailBlocksRendered(fx.blocks));
}

} // namespace audioapi::test
