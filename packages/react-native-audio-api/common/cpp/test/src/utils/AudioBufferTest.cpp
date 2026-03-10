#include <audioapi/utils/AudioBuffer.h>
#include <gtest/gtest.h>

#include <cmath>
#include <utility>
#include <vector>

using namespace audioapi;

static const float SQRT_HALF = sqrtf(0.5f);
static constexpr size_t BUF_SIZE = 10;
static constexpr float SR = 44100.0f;

class AudioBufferTest : public ::testing::Test {
 protected:
  static void fillChannel(AudioBuffer &buf, size_t ch, float value) {
    auto *channel = buf.getChannel(ch);
    for (size_t i = 0; i < buf.getSize(); ++i) {
      (*channel)[i] = value;
    }
  }

  static void fillAllChannels(AudioBuffer &buf, float value) {
    for (size_t ch = 0; ch < buf.getNumberOfChannels(); ++ch) {
      fillChannel(buf, ch, value);
    }
  }

  static void expectChannel(const AudioBuffer &buf, size_t ch, float expected, float tol = 0.0f) {
    auto *channel = buf.getChannel(ch);
    for (size_t i = 0; i < buf.getSize(); ++i) {
      if (tol > 0.0f) {
        EXPECT_NEAR((*channel)[i], expected, tol) << "ch=" << ch << " i=" << i;
      } else {
        EXPECT_FLOAT_EQ((*channel)[i], expected) << "ch=" << ch << " i=" << i;
      }
    }
  }
};

// ---------------------------------------------------------------------------
// Construction & Properties
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, ConstructorSetsProperties) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  EXPECT_EQ(buf.getSize(), BUF_SIZE);
  EXPECT_EQ(buf.getNumberOfChannels(), 2u);
  EXPECT_FLOAT_EQ(buf.getSampleRate(), SR);
}

TEST_F(AudioBufferTest, DurationIsCorrect) {
  AudioBuffer buf(static_cast<size_t>(SR), 1, SR);
  EXPECT_DOUBLE_EQ(buf.getDuration(), 1.0);

  AudioBuffer buf2(static_cast<size_t>(SR * 2), 1, SR);
  EXPECT_DOUBLE_EQ(buf2.getDuration(), 2.0);
}

TEST_F(AudioBufferTest, ChannelsInitializedToZero) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  expectChannel(buf, 0, 0.0f);
  expectChannel(buf, 1, 0.0f);
}

// ---------------------------------------------------------------------------
// Copy & Move semantics
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, CopyConstructorDeepCopies) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 2.0f);

  AudioBuffer dst(src);
  EXPECT_EQ(dst.getNumberOfChannels(), 2u);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 2.0f);

  fillChannel(dst, 0, 99.0f);
  expectChannel(src, 0, 1.0f);
}

TEST_F(AudioBufferTest, MoveConstructorTransfersOwnership) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 3.0f);

  AudioBuffer dst(std::move(src));
  EXPECT_EQ(dst.getSize(), BUF_SIZE);
  EXPECT_EQ(dst.getNumberOfChannels(), 2u);
  expectChannel(dst, 0, 3.0f);

  EXPECT_EQ(src.getSize(), 0u);
  EXPECT_EQ(src.getNumberOfChannels(), 0u);
}

TEST_F(AudioBufferTest, CopyAssignment) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 5.0f);
  fillChannel(src, 1, 6.0f);

  AudioBuffer dst(64, 1, 22050.0f);
  dst = src;
  EXPECT_EQ(dst.getSize(), BUF_SIZE);
  EXPECT_EQ(dst.getNumberOfChannels(), 2u);
  EXPECT_FLOAT_EQ(dst.getSampleRate(), SR);
  expectChannel(dst, 0, 5.0f);
  expectChannel(dst, 1, 6.0f);
}

TEST_F(AudioBufferTest, CopyAssignmentSameChannelCount) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 5.0f);
  fillChannel(src, 1, 6.0f);

  AudioBuffer dst(BUF_SIZE, 2, SR);
  dst = src;
  expectChannel(dst, 0, 5.0f);
  expectChannel(dst, 1, 6.0f);
}

TEST_F(AudioBufferTest, MoveAssignment) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 7.0f);

  AudioBuffer dst(64, 1, 22050.0f);
  dst = std::move(src);
  EXPECT_EQ(dst.getSize(), BUF_SIZE);
  EXPECT_EQ(dst.getNumberOfChannels(), 2u);
  expectChannel(dst, 0, 7.0f);

  EXPECT_EQ(src.getSize(), 0u);
  EXPECT_EQ(src.getNumberOfChannels(), 0u);
}

// ---------------------------------------------------------------------------
// getChannelByType
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, GetChannelByType_Stereo) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  fillChannel(buf, 0, 1.0f);
  fillChannel(buf, 1, 2.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelLeft))[0], 1.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelRight))[0], 2.0f);
}

TEST_F(AudioBufferTest, GetChannelByType_5_1) {
  AudioBuffer buf(BUF_SIZE, 6, SR);
  for (size_t ch = 0; ch < 6; ++ch) {
    fillChannel(buf, ch, static_cast<float>(ch));
  }
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelLeft))[0], 0.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelRight))[0], 1.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelCenter))[0], 2.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelLFE))[0], 3.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelSurroundLeft))[0], 4.0f);
  EXPECT_FLOAT_EQ((*buf.getChannelByType(AudioBuffer::ChannelSurroundRight))[0], 5.0f);
}

TEST_F(AudioBufferTest, GetChannelByType_ReturnsNullForUnknownLayout) {
  AudioBuffer buf(BUF_SIZE, 3, SR);
  EXPECT_EQ(buf.getChannelByType(AudioBuffer::ChannelLeft), nullptr);
}

// ---------------------------------------------------------------------------
// operator[]
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, OperatorBracket) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  buf[0][0] = 42.0f;
  buf[1][0] = 99.0f;
  EXPECT_FLOAT_EQ(buf[0][0], 42.0f);
  EXPECT_FLOAT_EQ(buf[1][0], 99.0f);

  const AudioBuffer &cbuf = buf;
  EXPECT_FLOAT_EQ(cbuf[0][0], 42.0f);
}

// ---------------------------------------------------------------------------
// Zero
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, ZeroEntireBuffer) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  fillAllChannels(buf, 42.0f);
  buf.zero();
  expectChannel(buf, 0, 0.0f);
  expectChannel(buf, 1, 0.0f);
}

TEST_F(AudioBufferTest, ZeroPartialRange) {
  AudioBuffer buf(BUF_SIZE, 1, SR);
  fillChannel(buf, 0, 1.0f);
  buf.zero(0, BUF_SIZE / 2);

  auto *ch = buf.getChannel(0);
  for (size_t i = 0; i < BUF_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], 0.0f);
  }
  for (size_t i = BUF_SIZE / 2; i < BUF_SIZE; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], 1.0f);
  }
}

// ---------------------------------------------------------------------------
// Scale & Normalize
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, ScaleMultipliesAllSamples) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  fillChannel(buf, 0, 2.0f);
  fillChannel(buf, 1, -3.0f);
  buf.scale(0.5f);
  expectChannel(buf, 0, 1.0f);
  expectChannel(buf, 1, -1.5f);
}

TEST_F(AudioBufferTest, NormalizeScalesToUnitPeak) {
  AudioBuffer buf(BUF_SIZE, 1, SR);
  fillChannel(buf, 0, 4.0f);
  buf.normalize();
  expectChannel(buf, 0, 1.0f);
}

// ---------------------------------------------------------------------------
// Sum — same channel count
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, SumSameChannelCount) {
  AudioBuffer dst(BUF_SIZE, 2, SR);
  AudioBuffer src(BUF_SIZE, 2, SR);
  fillChannel(dst, 0, 1.0f);
  fillChannel(dst, 1, 2.0f);
  fillChannel(src, 0, 10.0f);
  fillChannel(src, 1, 20.0f);

  dst.sum(src);
  expectChannel(dst, 0, 11.0f);
  expectChannel(dst, 1, 22.0f);
}

TEST_F(AudioBufferTest, SumSelfIsNoop) {
  AudioBuffer buf(BUF_SIZE, 1, SR);
  fillChannel(buf, 0, 5.0f);
  buf.sum(buf);
  expectChannel(buf, 0, 5.0f);
}

TEST_F(AudioBufferTest, SumPartialRange) {
  AudioBuffer dst(BUF_SIZE, 1, SR);
  AudioBuffer src(BUF_SIZE, 1, SR);
  fillChannel(src, 0, 1.0f);

  dst.sum(src, 0, 0, BUF_SIZE / 2);
  auto *ch = dst.getChannel(0);
  for (size_t i = 0; i < BUF_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], 1.0f);
  }
  for (size_t i = BUF_SIZE / 2; i < BUF_SIZE; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], 0.0f);
  }
}

TEST_F(AudioBufferTest, SumAccumulatesMultipleSources) {
  AudioBuffer dst(BUF_SIZE, 1, SR);
  AudioBuffer a(BUF_SIZE, 1, SR);
  AudioBuffer b(BUF_SIZE, 1, SR);
  fillChannel(a, 0, 1.0f);
  fillChannel(b, 0, 2.0f);

  dst.sum(a);
  dst.sum(b);
  expectChannel(dst, 0, 3.0f);
}

// ---------------------------------------------------------------------------
// Up-mixing (SPEAKERS interpretation)
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, UpMix_1_to_2) {
  AudioBuffer src(BUF_SIZE, 1, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 1.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 1.0f);
}

TEST_F(AudioBufferTest, UpMix_1_to_4) {
  AudioBuffer src(BUF_SIZE, 1, SR);
  AudioBuffer dst(BUF_SIZE, 4, SR);
  fillChannel(src, 0, 1.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 1.0f);
  expectChannel(dst, 2, 0.0f);
  expectChannel(dst, 3, 0.0f);
}

TEST_F(AudioBufferTest, UpMix_1_to_6) {
  AudioBuffer src(BUF_SIZE, 1, SR);
  AudioBuffer dst(BUF_SIZE, 6, SR);
  fillChannel(src, 0, 1.0f);

  dst.sum(src);
  expectChannel(dst, 0, 0.0f);
  expectChannel(dst, 1, 0.0f);
  expectChannel(dst, 2, 1.0f); // Center
  expectChannel(dst, 3, 0.0f);
  expectChannel(dst, 4, 0.0f);
  expectChannel(dst, 5, 0.0f);
}

TEST_F(AudioBufferTest, UpMix_2_to_4) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  AudioBuffer dst(BUF_SIZE, 4, SR);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 2.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 2.0f);
  expectChannel(dst, 2, 0.0f);
  expectChannel(dst, 3, 0.0f);
}

TEST_F(AudioBufferTest, UpMix_2_to_6) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  AudioBuffer dst(BUF_SIZE, 6, SR);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 2.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 2.0f);
  expectChannel(dst, 2, 0.0f);
  expectChannel(dst, 3, 0.0f);
  expectChannel(dst, 4, 0.0f);
  expectChannel(dst, 5, 0.0f);
}

TEST_F(AudioBufferTest, UpMix_4_to_6) {
  AudioBuffer src(BUF_SIZE, 4, SR);
  AudioBuffer dst(BUF_SIZE, 6, SR);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 2.0f);
  fillChannel(src, 2, 3.0f);
  fillChannel(src, 3, 4.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 2.0f);
  expectChannel(dst, 2, 0.0f);
  expectChannel(dst, 3, 0.0f);
  expectChannel(dst, 4, 3.0f);
  expectChannel(dst, 5, 4.0f);
}

// ---------------------------------------------------------------------------
// Down-mixing (SPEAKERS interpretation)
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, DownMix_2_to_1) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  AudioBuffer dst(BUF_SIZE, 1, SR);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 1.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f); // 0.5 + 0.5
}

TEST_F(AudioBufferTest, DownMix_2_to_1_Asymmetric) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  AudioBuffer dst(BUF_SIZE, 1, SR);
  fillChannel(src, 0, 2.0f);
  fillChannel(src, 1, 4.0f);

  dst.sum(src);
  expectChannel(dst, 0, 3.0f); // 0.5*2 + 0.5*4
}

TEST_F(AudioBufferTest, DownMix_4_to_1) {
  AudioBuffer src(BUF_SIZE, 4, SR);
  AudioBuffer dst(BUF_SIZE, 1, SR);
  fillAllChannels(src, 1.0f);

  dst.sum(src);
  expectChannel(dst, 0, 1.0f); // 0.25 * 4
}

TEST_F(AudioBufferTest, DownMix_6_to_1) {
  AudioBuffer src(BUF_SIZE, 6, SR);
  AudioBuffer dst(BUF_SIZE, 1, SR);
  fillAllChannels(src, 1.0f);
  fillChannel(src, 3, 0.0f); // LFE not part of the formula

  dst.sum(src);
  // sqrt(0.5)*L + sqrt(0.5)*R + C + 0.5*SL + 0.5*SR
  float expected = 2.0f * SQRT_HALF + 1.0f + 2.0f * 0.5f;
  expectChannel(dst, 0, expected, 1e-6f);
}

TEST_F(AudioBufferTest, DownMix_4_to_2) {
  AudioBuffer src(BUF_SIZE, 4, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillAllChannels(src, 1.0f);

  dst.sum(src);
  // L += 0.5*(L + SL) = 1.0, R += 0.5*(R + SR) = 1.0
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 1.0f);
}

TEST_F(AudioBufferTest, DownMix_6_to_2) {
  AudioBuffer src(BUF_SIZE, 6, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillAllChannels(src, 1.0f);
  fillChannel(src, 3, 0.0f); // LFE

  dst.sum(src);
  // L += L + sqrt(0.5)*(C + SL), R += R + sqrt(0.5)*(C + SR)
  float expected = 1.0f + 2.0f * SQRT_HALF;
  expectChannel(dst, 0, expected, 1e-6f);
  expectChannel(dst, 1, expected, 1e-6f);
}

TEST_F(AudioBufferTest, DownMix_6_to_4) {
  AudioBuffer src(BUF_SIZE, 6, SR);
  AudioBuffer dst(BUF_SIZE, 4, SR);
  fillAllChannels(src, 1.0f);
  fillChannel(src, 3, 0.0f); // LFE

  dst.sum(src);
  // L += L + sqrt(0.5)*C, R += R + sqrt(0.5)*C
  float expectedLR = 1.0f + SQRT_HALF;
  expectChannel(dst, 0, expectedLR, 1e-6f);
  expectChannel(dst, 1, expectedLR, 1e-6f);
  // SL += SL, SR += SR
  expectChannel(dst, 2, 1.0f);
  expectChannel(dst, 3, 1.0f);
}

// ---------------------------------------------------------------------------
// Discrete interpretation
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, DiscreteSum_SourceMoreChannels) {
  AudioBuffer src(BUF_SIZE, 4, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 2.0f);
  fillChannel(src, 2, 3.0f);
  fillChannel(src, 3, 4.0f);

  dst.sum(src, ChannelInterpretation::DISCRETE);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 2.0f);
}

TEST_F(AudioBufferTest, DiscreteSum_SourceFewerChannels) {
  AudioBuffer src(BUF_SIZE, 1, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillChannel(dst, 1, 5.0f);
  fillChannel(src, 0, 1.0f);

  dst.sum(src, ChannelInterpretation::DISCRETE);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 5.0f); // untouched
}

// ---------------------------------------------------------------------------
// Copy (including up/down-mix via zero+sum)
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, CopySameChannelCount) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillAllChannels(dst, 99.0f);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 2.0f);

  dst.copy(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 2.0f);
}

TEST_F(AudioBufferTest, CopyWithUpMixing) {
  AudioBuffer src(BUF_SIZE, 1, SR);
  AudioBuffer dst(BUF_SIZE, 2, SR);
  fillAllChannels(dst, 99.0f);
  fillChannel(src, 0, 1.0f);

  dst.copy(src);
  expectChannel(dst, 0, 1.0f);
  expectChannel(dst, 1, 1.0f);
}

TEST_F(AudioBufferTest, CopyWithDownMixing) {
  AudioBuffer src(BUF_SIZE, 2, SR);
  AudioBuffer dst(BUF_SIZE, 1, SR);
  fillChannel(dst, 0, 99.0f);
  fillChannel(src, 0, 1.0f);
  fillChannel(src, 1, 1.0f);

  dst.copy(src);
  expectChannel(dst, 0, 1.0f); // 0.5 + 0.5
}

TEST_F(AudioBufferTest, CopySelfIsNoop) {
  AudioBuffer buf(BUF_SIZE, 1, SR);
  fillChannel(buf, 0, 7.0f);
  buf.copy(buf);
  expectChannel(buf, 0, 7.0f);
}

TEST_F(AudioBufferTest, CopyPartialRange) {
  AudioBuffer src(BUF_SIZE, 1, SR);
  AudioBuffer dst(BUF_SIZE, 1, SR);
  fillChannel(dst, 0, 99.0f);
  fillChannel(src, 0, 1.0f);

  dst.copy(src, 0, 0, BUF_SIZE / 2);
  auto *ch = dst.getChannel(0);
  for (size_t i = 0; i < BUF_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], 1.0f);
  }
  for (size_t i = BUF_SIZE / 2; i < BUF_SIZE; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], 99.0f);
  }
}

// ---------------------------------------------------------------------------
// Interleave / Deinterleave
// ---------------------------------------------------------------------------

TEST_F(AudioBufferTest, DeinterleaveMono) {
  constexpr size_t FRAMES = 4;
  AudioBuffer buf(FRAMES, 1, SR);
  float data[] = {10, 20, 30, 40};

  buf.deinterleaveFrom(data, FRAMES);
  auto *ch = buf.getChannel(0);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_FLOAT_EQ((*ch)[i], data[i]);
  }
}

TEST_F(AudioBufferTest, DeinterleaveStereo) {
  constexpr size_t FRAMES = 4;
  AudioBuffer buf(FRAMES, 2, SR);
  float interleaved[] = {1, 2, 3, 4, 5, 6, 7, 8};

  buf.deinterleaveFrom(interleaved, FRAMES);
  auto *left = buf.getChannel(0);
  auto *right = buf.getChannel(1);
  EXPECT_FLOAT_EQ((*left)[0], 1.0f);
  EXPECT_FLOAT_EQ((*left)[1], 3.0f);
  EXPECT_FLOAT_EQ((*left)[2], 5.0f);
  EXPECT_FLOAT_EQ((*left)[3], 7.0f);
  EXPECT_FLOAT_EQ((*right)[0], 2.0f);
  EXPECT_FLOAT_EQ((*right)[1], 4.0f);
  EXPECT_FLOAT_EQ((*right)[2], 6.0f);
  EXPECT_FLOAT_EQ((*right)[3], 8.0f);
}

TEST_F(AudioBufferTest, InterleaveStereo) {
  constexpr size_t FRAMES = 4;
  AudioBuffer buf(FRAMES, 2, SR);
  auto *left = buf.getChannel(0);
  auto *right = buf.getChannel(1);
  for (size_t i = 0; i < FRAMES; ++i) {
    (*left)[i] = static_cast<float>(2 * i + 1);
    (*right)[i] = static_cast<float>(2 * i + 2);
  }

  float interleaved[8] = {};
  buf.interleaveTo(interleaved, FRAMES);
  for (size_t i = 0; i < FRAMES * 2; ++i) {
    EXPECT_FLOAT_EQ(interleaved[i], static_cast<float>(i + 1));
  }
}

TEST_F(AudioBufferTest, InterleaveDeinterleaveRoundtrip) {
  constexpr size_t FRAMES = 64;
  AudioBuffer buf(FRAMES, 2, SR);
  auto *left = buf.getChannel(0);
  auto *right = buf.getChannel(1);
  for (size_t i = 0; i < FRAMES; ++i) {
    (*left)[i] = static_cast<float>(i);
    (*right)[i] = static_cast<float>(i + 100);
  }

  std::vector<float> interleaved(FRAMES * 2);
  buf.interleaveTo(interleaved.data(), FRAMES);

  AudioBuffer buf2(FRAMES, 2, SR);
  buf2.deinterleaveFrom(interleaved.data(), FRAMES);

  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_FLOAT_EQ((*buf2.getChannel(0))[i], static_cast<float>(i));
    EXPECT_FLOAT_EQ((*buf2.getChannel(1))[i], static_cast<float>(i + 100));
  }
}

TEST_F(AudioBufferTest, DeinterleaveMultichannel) {
  constexpr size_t FRAMES = 4;
  constexpr int CHANNELS = 4;
  AudioBuffer buf(FRAMES, CHANNELS, SR);
  // [ch0_f0, ch1_f0, ch2_f0, ch3_f0, ch0_f1, ...]
  float interleaved[FRAMES * CHANNELS];
  for (size_t i = 0; i < FRAMES * CHANNELS; ++i) {
    interleaved[i] = static_cast<float>(i);
  }

  buf.deinterleaveFrom(interleaved, FRAMES);
  for (size_t f = 0; f < FRAMES; ++f) {
    for (size_t ch = 0; ch < CHANNELS; ++ch) {
      EXPECT_FLOAT_EQ((*buf.getChannel(ch))[f], static_cast<float>(f * CHANNELS + ch));
    }
  }
}

TEST_F(AudioBufferTest, DeinterleaveZeroFramesIsNoop) {
  AudioBuffer buf(BUF_SIZE, 2, SR);
  fillAllChannels(buf, 42.0f);
  float dummy[] = {999.0f, 999.0f};
  buf.deinterleaveFrom(dummy, 0);
  expectChannel(buf, 0, 42.0f);
}
