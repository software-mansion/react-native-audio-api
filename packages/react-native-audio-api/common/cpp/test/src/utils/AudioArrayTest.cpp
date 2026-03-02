#include <audioapi/utils/AudioArray.h>
#include <gtest/gtest.h>

#include <numeric>
#include <utility>
#include <vector>

using namespace audioapi;

static constexpr size_t ARR_SIZE = 128;

class AudioArrayTest : public ::testing::Test {
 protected:
  static void fill(AudioArray &arr, float value) {
    for (size_t i = 0; i < arr.getSize(); ++i) {
      arr[i] = value;
    }
  }

  static void fillRamp(AudioArray &arr) {
    for (size_t i = 0; i < arr.getSize(); ++i) {
      arr[i] = static_cast<float>(i + 1);
    }
  }

  static void expectAll(const AudioArray &arr, float expected, float tol = 0.0f) {
    for (size_t i = 0; i < arr.getSize(); ++i) {
      if (tol > 0.0f) {
        EXPECT_NEAR(arr[i], expected, tol) << "i=" << i;
      } else {
        EXPECT_FLOAT_EQ(arr[i], expected) << "i=" << i;
      }
    }
  }
};

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, SizeConstructorInitializesToZero) {
  AudioArray arr(ARR_SIZE);
  EXPECT_EQ(arr.getSize(), ARR_SIZE);
  expectAll(arr, 0.0f);
}

TEST_F(AudioArrayTest, DataConstructorCopiesInput) {
  std::vector<float> data(ARR_SIZE);
  std::iota(data.begin(), data.end(), 1.0f);

  AudioArray arr(data.data(), data.size());
  EXPECT_EQ(arr.getSize(), ARR_SIZE);
  for (size_t i = 0; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(arr[i], static_cast<float>(i + 1));
  }

  data[0] = 999.0f;
  EXPECT_FLOAT_EQ(arr[0], 1.0f);
}

TEST_F(AudioArrayTest, ZeroSizeConstructor) {
  AudioArray arr(0);
  EXPECT_EQ(arr.getSize(), 0u);
}

// ---------------------------------------------------------------------------
// Copy & Move
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, CopyConstructorDeepCopies) {
  AudioArray src(ARR_SIZE);
  fillRamp(src);

  AudioArray dst(src);
  EXPECT_EQ(dst.getSize(), ARR_SIZE);
  for (size_t i = 0; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(dst[i], src[i]);
  }

  dst[0] = 999.0f;
  EXPECT_FLOAT_EQ(src[0], 1.0f);
}

TEST_F(AudioArrayTest, MoveConstructorTransfersOwnership) {
  AudioArray src(ARR_SIZE);
  fill(src, 3.0f);

  AudioArray dst(std::move(src));
  EXPECT_EQ(dst.getSize(), ARR_SIZE);
  expectAll(dst, 3.0f);
  EXPECT_EQ(src.getSize(), 0u);
}

TEST_F(AudioArrayTest, CopyAssignment) {
  AudioArray src(ARR_SIZE);
  fill(src, 5.0f);

  AudioArray dst(16);
  dst = src;
  EXPECT_EQ(dst.getSize(), ARR_SIZE);
  expectAll(dst, 5.0f);
}

TEST_F(AudioArrayTest, CopyAssignmentSameSize) {
  AudioArray src(ARR_SIZE);
  fill(src, 7.0f);

  AudioArray dst(ARR_SIZE);
  dst = src;
  expectAll(dst, 7.0f);
}

TEST_F(AudioArrayTest, MoveAssignment) {
  AudioArray src(ARR_SIZE);
  fill(src, 9.0f);

  AudioArray dst(16);
  dst = std::move(src);
  EXPECT_EQ(dst.getSize(), ARR_SIZE);
  expectAll(dst, 9.0f);
  EXPECT_EQ(src.getSize(), 0u);
}

TEST_F(AudioArrayTest, SelfCopyAssignmentIsNoop) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 1.0f);
  arr = arr;
  expectAll(arr, 1.0f);
}

// ---------------------------------------------------------------------------
// operator[], begin/end, span
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, OperatorBracketReadWrite) {
  AudioArray arr(ARR_SIZE);
  arr[0] = 42.0f;
  arr[ARR_SIZE - 1] = 99.0f;
  EXPECT_FLOAT_EQ(arr[0], 42.0f);
  EXPECT_FLOAT_EQ(arr[ARR_SIZE - 1], 99.0f);
}

TEST_F(AudioArrayTest, BeginEndIterators) {
  AudioArray arr(ARR_SIZE);
  fillRamp(arr);

  EXPECT_EQ(static_cast<size_t>(arr.end() - arr.begin()), ARR_SIZE);
  EXPECT_FLOAT_EQ(*arr.begin(), 1.0f);
  EXPECT_FLOAT_EQ(*(arr.end() - 1), static_cast<float>(ARR_SIZE));
}

TEST_F(AudioArrayTest, SpanCoversFullArray) {
  AudioArray arr(ARR_SIZE);
  fillRamp(arr);
  auto s = arr.span();
  EXPECT_EQ(s.size(), ARR_SIZE);
  EXPECT_FLOAT_EQ(s[0], 1.0f);
}

TEST_F(AudioArrayTest, SubSpanReturnsCorrectRange) {
  AudioArray arr(ARR_SIZE);
  fillRamp(arr);

  auto sub = arr.subSpan(4, 10);
  EXPECT_EQ(sub.size(), 4u);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_FLOAT_EQ(sub[i], static_cast<float>(11 + i));
  }
}

TEST_F(AudioArrayTest, SubSpanThrowsOnOutOfRange) {
  AudioArray arr(ARR_SIZE);
  EXPECT_THROW(auto x = arr.subSpan(ARR_SIZE, 1), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Zero
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, ZeroEntireArray) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 42.0f);
  arr.zero();
  expectAll(arr, 0.0f);
}

TEST_F(AudioArrayTest, ZeroPartialRange) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 1.0f);
  arr.zero(0, ARR_SIZE / 2);

  for (size_t i = 0; i < ARR_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ(arr[i], 0.0f);
  }
  for (size_t i = ARR_SIZE / 2; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(arr[i], 1.0f);
  }
}

// ---------------------------------------------------------------------------
// Sum
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, SumFullArrayDefaultGain) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(dst, 1.0f);
  fill(src, 2.0f);

  dst.sum(src);
  expectAll(dst, 3.0f);
}

TEST_F(AudioArrayTest, SumFullArrayWithGain) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(dst, 1.0f);
  fill(src, 4.0f);

  dst.sum(src, 0.5f);
  expectAll(dst, 3.0f);
}

TEST_F(AudioArrayTest, SumPartialRange) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(src, 1.0f);

  dst.sum(src, 0, 0, ARR_SIZE / 2);
  for (size_t i = 0; i < ARR_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ(dst[i], 1.0f);
  }
  for (size_t i = ARR_SIZE / 2; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(dst[i], 0.0f);
  }
}

TEST_F(AudioArrayTest, SumWithOffset) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(src, 5.0f);

  dst.sum(src, 0, ARR_SIZE / 2, ARR_SIZE / 2);
  for (size_t i = 0; i < ARR_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ(dst[i], 0.0f);
  }
  for (size_t i = ARR_SIZE / 2; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(dst[i], 5.0f);
  }
}

TEST_F(AudioArrayTest, SumThrowsOnOutOfRange) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  EXPECT_THROW(dst.sum(src, 0, 0, ARR_SIZE + 1), std::out_of_range);
}

TEST_F(AudioArrayTest, SumAccumulatesMultipleCalls) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(src, 1.0f);

  dst.sum(src);
  dst.sum(src);
  dst.sum(src);
  expectAll(dst, 3.0f);
}

// ---------------------------------------------------------------------------
// Multiply
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, MultiplyFullArray) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(dst, 3.0f);
  fill(src, 2.0f);

  dst.multiply(src);
  expectAll(dst, 6.0f);
}

TEST_F(AudioArrayTest, MultiplyPartialLength) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(dst, 3.0f);
  fill(src, 2.0f);

  dst.multiply(src, ARR_SIZE / 2);
  for (size_t i = 0; i < ARR_SIZE / 2; ++i) {
    EXPECT_FLOAT_EQ(dst[i], 6.0f);
  }
  for (size_t i = ARR_SIZE / 2; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(dst[i], 3.0f);
  }
}

TEST_F(AudioArrayTest, MultiplyByZero) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE);
  fill(dst, 5.0f);
  fill(src, 0.0f);

  dst.multiply(src);
  expectAll(dst, 0.0f);
}

TEST_F(AudioArrayTest, MultiplyThrowsOnOutOfRange) {
  AudioArray dst(ARR_SIZE);
  AudioArray src(ARR_SIZE / 2);
  EXPECT_THROW(dst.multiply(src), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Copy (AudioArray source)
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, CopyFullArray) {
  AudioArray src(ARR_SIZE);
  AudioArray dst(ARR_SIZE);
  fillRamp(src);
  fill(dst, 99.0f);

  dst.copy(src);
  for (size_t i = 0; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(dst[i], static_cast<float>(i + 1));
  }
}

TEST_F(AudioArrayTest, CopyPartialRange) {
  AudioArray src(ARR_SIZE);
  AudioArray dst(ARR_SIZE);
  fillRamp(src);

  dst.copy(src, 10, 20, 5);
  for (size_t i = 0; i < 5; ++i) {
    EXPECT_FLOAT_EQ(dst[20 + i], static_cast<float>(11 + i));
  }
  EXPECT_FLOAT_EQ(dst[0], 0.0f);
}

TEST_F(AudioArrayTest, CopyThrowsWhenSourceTooSmall) {
  AudioArray src(4);
  AudioArray dst(ARR_SIZE);
  EXPECT_THROW(dst.copy(src, 0, 0, 8), std::out_of_range);
}

TEST_F(AudioArrayTest, CopyThrowsWhenDestinationTooSmall) {
  AudioArray src(ARR_SIZE);
  AudioArray dst(4);
  EXPECT_THROW(dst.copy(src, 0, 0, 8), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Copy (raw float pointer)
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, CopyFromRawPointer) {
  float data[] = {10, 20, 30, 40};
  AudioArray dst(4);

  dst.copy(data, 0, 0, 4);
  for (size_t i = 0; i < 4; ++i) {
    EXPECT_FLOAT_EQ(dst[i], data[i]);
  }
}

TEST_F(AudioArrayTest, CopyFromRawPointerWithOffset) {
  float data[] = {10, 20, 30, 40, 50};
  AudioArray dst(8);

  dst.copy(data, 2, 1, 3);
  EXPECT_FLOAT_EQ(dst[0], 0.0f);
  EXPECT_FLOAT_EQ(dst[1], 30.0f);
  EXPECT_FLOAT_EQ(dst[2], 40.0f);
  EXPECT_FLOAT_EQ(dst[3], 50.0f);
}

// ---------------------------------------------------------------------------
// CopyReverse
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, CopyReverse) {
  AudioArray src(8);
  AudioArray dst(8);
  for (size_t i = 0; i < 8; ++i) {
    src[i] = static_cast<float>(i + 1);
  }

  // sourceStart=4 means we start at index 4 and go backwards
  dst.copyReverse(src, 4, 0, 4);
  EXPECT_FLOAT_EQ(dst[0], 5.0f);
  EXPECT_FLOAT_EQ(dst[1], 4.0f);
  EXPECT_FLOAT_EQ(dst[2], 3.0f);
  EXPECT_FLOAT_EQ(dst[3], 2.0f);
}

// ---------------------------------------------------------------------------
// CopyTo (raw float pointer)
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, CopyToRawPointer) {
  AudioArray src(ARR_SIZE);
  fillRamp(src);

  std::vector<float> dest(ARR_SIZE, 0.0f);
  src.copyTo(dest.data(), 0, 0, ARR_SIZE);
  for (size_t i = 0; i < ARR_SIZE; ++i) {
    EXPECT_FLOAT_EQ(dest[i], static_cast<float>(i + 1));
  }
}

TEST_F(AudioArrayTest, CopyToWithOffsets) {
  AudioArray src(8);
  fillRamp(src);

  float dest[16] = {};
  src.copyTo(dest, 2, 4, 3);
  EXPECT_FLOAT_EQ(dest[4], 3.0f);
  EXPECT_FLOAT_EQ(dest[5], 4.0f);
  EXPECT_FLOAT_EQ(dest[6], 5.0f);
  EXPECT_FLOAT_EQ(dest[0], 0.0f);
}

TEST_F(AudioArrayTest, CopyToThrowsOnOutOfRange) {
  AudioArray src(4);
  float dest[4] = {};
  EXPECT_THROW(src.copyTo(dest, 2, 0, 4), std::out_of_range);
}

// ---------------------------------------------------------------------------
// CopyWithin
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, CopyWithinForward) {
  AudioArray arr(8);
  for (size_t i = 0; i < 8; ++i) {
    arr[i] = static_cast<float>(i + 1);
  }

  arr.copyWithin(0, 4, 4);
  EXPECT_FLOAT_EQ(arr[4], 1.0f);
  EXPECT_FLOAT_EQ(arr[5], 2.0f);
  EXPECT_FLOAT_EQ(arr[6], 3.0f);
  EXPECT_FLOAT_EQ(arr[7], 4.0f);
}

TEST_F(AudioArrayTest, CopyWithinOverlapping) {
  AudioArray arr(8);
  for (size_t i = 0; i < 8; ++i) {
    arr[i] = static_cast<float>(i + 1);
  }

  // memmove handles overlapping regions correctly
  arr.copyWithin(2, 0, 4);
  EXPECT_FLOAT_EQ(arr[0], 3.0f);
  EXPECT_FLOAT_EQ(arr[1], 4.0f);
  EXPECT_FLOAT_EQ(arr[2], 5.0f);
  EXPECT_FLOAT_EQ(arr[3], 6.0f);
}

TEST_F(AudioArrayTest, CopyWithinThrowsOnOutOfRange) {
  AudioArray arr(8);
  EXPECT_THROW(arr.copyWithin(0, 4, 8), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Reverse
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, ReverseArray) {
  AudioArray arr(8);
  for (size_t i = 0; i < 8; ++i) {
    arr[i] = static_cast<float>(i + 1);
  }

  arr.reverse();
  for (size_t i = 0; i < 8; ++i) {
    EXPECT_FLOAT_EQ(arr[i], static_cast<float>(8 - i));
  }
}

TEST_F(AudioArrayTest, ReverseSingleElement) {
  AudioArray arr(1);
  arr[0] = 42.0f;
  arr.reverse();
  EXPECT_FLOAT_EQ(arr[0], 42.0f);
}

// ---------------------------------------------------------------------------
// Scale
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, Scale) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 2.0f);
  arr.scale(3.0f);
  expectAll(arr, 6.0f);
}

TEST_F(AudioArrayTest, ScaleByZero) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 5.0f);
  arr.scale(0.0f);
  expectAll(arr, 0.0f);
}

TEST_F(AudioArrayTest, ScaleNegative) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 2.0f);
  arr.scale(-1.0f);
  expectAll(arr, -2.0f);
}

// ---------------------------------------------------------------------------
// Normalize
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, NormalizeScalesToUnit) {
  AudioArray arr(ARR_SIZE);
  fill(arr, 4.0f);
  arr.normalize();
  expectAll(arr, 1.0f);
}

TEST_F(AudioArrayTest, NormalizeWithNegativeValues) {
  AudioArray arr(4);
  arr[0] = -8.0f;
  arr[1] = 4.0f;
  arr[2] = -2.0f;
  arr[3] = 1.0f;

  arr.normalize();
  EXPECT_FLOAT_EQ(arr[0], -1.0f);
  EXPECT_FLOAT_EQ(arr[1], 0.5f);
  EXPECT_FLOAT_EQ(arr[2], -0.25f);
  EXPECT_FLOAT_EQ(arr[3], 0.125f);
}

TEST_F(AudioArrayTest, NormalizeAllZerosIsNoop) {
  AudioArray arr(ARR_SIZE);
  arr.normalize();
  expectAll(arr, 0.0f);
}

TEST_F(AudioArrayTest, NormalizeAlreadyNormalizedIsNoop) {
  AudioArray arr(4);
  arr[0] = 1.0f;
  arr[1] = -0.5f;
  arr[2] = 0.25f;
  arr[3] = -1.0f;

  arr.normalize();
  EXPECT_FLOAT_EQ(arr[0], 1.0f);
  EXPECT_FLOAT_EQ(arr[1], -0.5f);
  EXPECT_FLOAT_EQ(arr[2], 0.25f);
  EXPECT_FLOAT_EQ(arr[3], -1.0f);
}

// ---------------------------------------------------------------------------
// GetMaxAbsValue
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, GetMaxAbsValue) {
  AudioArray arr(4);
  arr[0] = -5.0f;
  arr[1] = 3.0f;
  arr[2] = 1.0f;
  arr[3] = -2.0f;
  EXPECT_FLOAT_EQ(arr.getMaxAbsValue(), 5.0f);
}

// ---------------------------------------------------------------------------
// ComputeConvolution
// ---------------------------------------------------------------------------

TEST_F(AudioArrayTest, ComputeConvolutionBasic) {
  AudioArray signal(4);
  signal[0] = 1.0f;
  signal[1] = 2.0f;
  signal[2] = 3.0f;
  signal[3] = 4.0f;

  AudioArray kernel(2);
  kernel[0] = 1.0f;
  kernel[1] = 0.5f;

  // convolution at startIndex=0: signal[0]*kernel[0] + signal[1]*kernel[1] = 1*1 + 2*0.5 = 2.0
  float result = signal.computeConvolution(kernel, 0);
  EXPECT_FLOAT_EQ(result, 2.0f);
}

TEST_F(AudioArrayTest, ComputeConvolutionWithOffset) {
  AudioArray signal(4);
  signal[0] = 1.0f;
  signal[1] = 2.0f;
  signal[2] = 3.0f;
  signal[3] = 4.0f;

  AudioArray kernel(2);
  kernel[0] = 1.0f;
  kernel[1] = 1.0f;

  // at startIndex=2: signal[2]*kernel[0] + signal[3]*kernel[1] = 3 + 4 = 7.0
  float result = signal.computeConvolution(kernel, 2);
  EXPECT_FLOAT_EQ(result, 7.0f);
}

TEST_F(AudioArrayTest, ComputeConvolutionThrowsOnOutOfRange) {
  AudioArray signal(4);
  AudioArray kernel(4);
  EXPECT_THROW(auto x = signal.computeConvolution(kernel, 2), std::out_of_range);
}
