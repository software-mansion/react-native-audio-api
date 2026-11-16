#include <audioapi/dsp/Convolver.h>
#include <audioapi/utils/AudioArray.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr float kTolerance = 1e-4f;

std::vector<float> randomSignal(size_t length, unsigned seed, float gain = 1.0f) {
  std::mt19937 rng(seed);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  std::vector<float> signal(length);
  for (auto &sample : signal) {
    sample = gain * dist(rng);
  }
  return signal;
}

std::vector<float> directConvolution(
    const std::vector<float> &input,
    const std::vector<float> &ir) {
  std::vector<float> output(input.size(), 0.0f);
  for (size_t n = 0; n < input.size(); ++n) {
    double acc = 0.0;
    const size_t taps = std::min(ir.size(), n + 1);
    for (size_t k = 0; k < taps; ++k) {
      acc += static_cast<double>(ir[k]) * input[n - k];
    }
    output[n] = static_cast<float>(acc);
  }
  return output;
}

std::vector<size_t> uniformChunks(size_t total, size_t chunk) {
  std::vector<size_t> chunks;
  for (size_t done = 0; done < total; done += chunk) {
    chunks.push_back(std::min(chunk, total - done));
  }
  return chunks;
}

std::vector<size_t> randomChunks(size_t total, size_t maxChunk, unsigned seed) {
  std::mt19937 rng(seed);
  std::uniform_int_distribution<size_t> dist(1, maxChunk);
  std::vector<size_t> chunks;
  for (size_t done = 0; done < total;) {
    const size_t chunk = std::min(dist(rng), total - done);
    chunks.push_back(chunk);
    done += chunk;
  }
  return chunks;
}

/// Streams @p input through a convolver in calls of @p chunks frames, each call through
/// quantum-sized scratch arrays like the node's buffers.
std::vector<float> convolveInChunks(
    const std::vector<float> &input,
    const std::vector<float> &ir,
    size_t blockSize,
    const std::vector<size_t> &chunks) {
  Convolver convolver;
  AudioArray irArray(ir.data(), ir.size());
  EXPECT_TRUE(convolver.init(blockSize, irArray, ir.size()));

  const size_t maxChunk = *std::max_element(chunks.begin(), chunks.end());
  DSPAudioArray in(maxChunk);
  DSPAudioArray out(maxChunk);
  std::vector<float> output;
  output.reserve(input.size());

  size_t position = 0;
  for (const size_t chunk : chunks) {
    in.zero();
    in.copy(input.data() + position, 0, 0, chunk);
    convolver.process(in, out, chunk);
    output.insert(output.end(), out.begin(), out.begin() + chunk);
    position += chunk;
  }
  return output;
}

void expectMatchesReference(const std::vector<float> &actual, const std::vector<float> &expected) {
  ASSERT_EQ(actual.size(), expected.size());
  float maxError = 0.0f;
  size_t worstFrame = 0;
  for (size_t i = 0; i < actual.size(); ++i) {
    const float error = std::fabs(actual[i] - expected[i]);
    if (error > maxError) {
      maxError = error;
      worstFrame = i;
    }
  }
  EXPECT_LT(maxError, kTolerance) << "worst frame " << worstFrame;
}

struct ChunkingCase {
  const char *name;
  size_t blockSize;
  size_t irLength;
  std::vector<size_t> chunks;
};

constexpr size_t kSignalLength = 8192;

class ConvolverChunkingTest : public ::testing::TestWithParam<ChunkingCase> {};

} // namespace

TEST_P(ConvolverChunkingTest, MatchesDirectConvolutionWithoutLatency) {
  const auto &param = GetParam();
  const auto input = randomSignal(kSignalLength, 1);
  const auto ir =
      randomSignal(param.irLength, 2, 1.0f / std::sqrt(static_cast<float>(param.irLength)));

  const auto output = convolveInChunks(input, ir, param.blockSize, param.chunks);

  expectMatchesReference(output, directConvolution(input, ir));
}

INSTANTIATE_TEST_SUITE_P(
    Convolver,
    ConvolverChunkingTest,
    ::testing::Values(
        ChunkingCase{"Uniform128Block128", 128, 3000, uniformChunks(kSignalLength, 128)},
        ChunkingCase{"Uniform441Block512", 512, 3000, uniformChunks(kSignalLength, 441)},
        ChunkingCase{"Uniform100Block128", 128, 3000, uniformChunks(kSignalLength, 100)},
        ChunkingCase{"Uniform1000Block128", 128, 3000, uniformChunks(kSignalLength, 1000)},
        ChunkingCase{"Uniform1Block128", 128, 300, uniformChunks(kSignalLength, 1)},
        ChunkingCase{"Uniform20Block32", 32, 1000, uniformChunks(kSignalLength, 20)},
        ChunkingCase{"RandomBlock32", 32, 1000, randomChunks(kSignalLength, 100, 8)},
        ChunkingCase{"IrShorterThanBlock", 512, 50, uniformChunks(kSignalLength, 441)},
        ChunkingCase{"RandomBlock128", 128, 3000, randomChunks(kSignalLength, 700, 3)},
        ChunkingCase{"RandomBlock512", 512, 5000, randomChunks(kSignalLength, 1500, 4)},
        ChunkingCase{
            "HardwareJitter470And471",
            512,
            4000,
            [] {
              std::vector<size_t> chunks;
              for (size_t done = 0; done < kSignalLength;) {
                const size_t chunk =
                    std::min<size_t>(chunks.size() % 2 == 0 ? 470 : 471, kSignalLength - done);
                chunks.push_back(chunk);
                done += chunk;
              }
              return chunks;
            }()}),
    [](const ::testing::TestParamInfo<ChunkingCase> &info) {
      return std::string(info.param.name);
    });

TEST(ConvolverTest, ChunkingDoesNotChangeTheOutput) {
  const auto input = randomSignal(kSignalLength, 5);
  const auto ir = randomSignal(2048, 6, 1.0f / std::sqrt(2048.0f));

  const auto uniform = convolveInChunks(input, ir, 512, uniformChunks(kSignalLength, 512));
  const auto irregular = convolveInChunks(input, ir, 512, randomChunks(kSignalLength, 900, 7));

  expectMatchesReference(irregular, uniform);
}

TEST(ConvolverTest, ZeroLengthIrOutputsSilence) {
  Convolver convolver;
  AudioArray ir(1);
  ASSERT_TRUE(convolver.init(128, ir, 0));

  DSPAudioArray in(200);
  DSPAudioArray out(200);
  for (size_t i = 0; i < in.getSize(); ++i) {
    in[i] = 1.0f;
    out[i] = 1.0f;
  }
  convolver.process(in, out, 200);

  for (size_t i = 0; i < out.getSize(); ++i) {
    EXPECT_EQ(out[i], 0.0f);
  }
}

// NOLINTEND
