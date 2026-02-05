/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 * Copyright (C) 2020, Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <audioapi/dsp/AudioUtils.hpp>
#include <audioapi/dsp/VectorMath.h>
#include <algorithm>

#if defined(HAVE_ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if defined(HAVE_X86_SSE2)
#include <emmintrin.h>
#endif

#if defined(HAVE_ARM_NEON_INTRINSICS)
#include <arm_neon.h>
#endif

namespace audioapi::dsp {

#if defined(HAVE_ACCELERATE)

void multiplyByScalar(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  vDSP_vsmul(inputVector, 1, &scalar, outputVector, 1, numberOfElementsToProcess);
}

void addScalar(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  vDSP_vsadd(inputVector, 1, &scalar, outputVector, 1, numberOfElementsToProcess);
}

void add(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  vDSP_vadd(inputVector1, 1, inputVector2, 1, outputVector, 1, numberOfElementsToProcess);
}

void subtract(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  vDSP_vsub(inputVector1, 1, inputVector2, 1, outputVector, 1, numberOfElementsToProcess);
}

void multiply(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  vDSP_vmul(inputVector1, 1, inputVector2, 1, outputVector, 1, numberOfElementsToProcess);
}

float maximumMagnitude(const float *inputVector, size_t numberOfElementsToProcess) {
  float maximumValue = 0;
  vDSP_maxmgv(inputVector, 1, &maximumValue, numberOfElementsToProcess);
  return maximumValue;
}

void multiplyByScalarThenAddToOutput(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  vDSP_vsma(inputVector, 1, &scalar, outputVector, 1, outputVector, 1, numberOfElementsToProcess);
}

float computeConvolution(const float *state, const float *kernel, size_t kernelSize) {
    float result = 0.0f;
    vDSP_conv(state, 1, kernel, 1, &result, 1, 1, kernelSize);
    return result;
}

void deinterleaveStereo(
        const float * __restrict inputInterleaved,
        float * __restrict outputLeft,
        float * __restrict outputRight,
        size_t numberOfFrames) {
    float zero = 0.0f;
    vDSP_vsadd(inputInterleaved, 2, &zero, outputLeft, 1, numberOfFrames);
    vDSP_vsadd(inputInterleaved + 1, 2, &zero, outputRight, 1, numberOfFrames);
}

void interleaveStereo(
    const float* __restrict inputLeft,
    const float* __restrict inputRight,
    float* __restrict outputInterleaved,
    size_t numberOfFrames) {
    float zero = 0.0f;
    vDSP_vsadd(inputLeft, 1, &zero, outputInterleaved, 2, numberOfFrames);
    vDSP_vsadd(inputRight, 1, &zero, outputInterleaved + 1, 2, numberOfFrames);
}

#else

#if defined(HAVE_X86_SSE2)
static inline bool is16ByteAligned(const float *vector) {
  return !(reinterpret_cast<uintptr_t>(vector) & 0x0F);
}
#endif

void multiplyByScalar(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;

#if defined(HAVE_X86_SSE2)

  // If the inputVector address is not 16-byte aligned, the first several frames
  // (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector) && n) {
    *outputVector = scalar * *inputVector;
    inputVector++;
    outputVector++;
    n--;
  }

  // Now the inputVector address is aligned and start to apply SSE.
  size_t group = n / 4;
  __m128 mScale = _mm_set_ps1(scalar);
  __m128 *pSource;
  __m128 *pDest;
  __m128 dest;

  if (!is16ByteAligned(outputVector)) {
    while (group--) {
      pSource = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector));
      dest = _mm_mul_ps(*pSource, mScale);
      _mm_storeu_ps(outputVector, dest);

      inputVector += 4;
      outputVector += 4;
    }
  } else {
    while (group--) {
      pSource = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector));
      pDest = reinterpret_cast<__m128 *>(outputVector);
      *pDest = _mm_mul_ps(*pSource, mScale);

      inputVector += 4;
      outputVector += 4;
    }
  }

  // Non-SSE handling for remaining frames which is less than 4.
  n %= 4;
#elif defined(HAVE_ARM_NEON_INTRINSICS)
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;

  while (outputVector < endP) {
    float32x4_t source = vld1q_f32(inputVector);
    vst1q_f32(outputVector, vmulq_n_f32(source, scalar));

    inputVector += 4;
    outputVector += 4;
  }
  n = tailFrames;
#endif
  while (n--) {
    *outputVector = scalar * *inputVector;
    ++inputVector;
    ++outputVector;
  }
}

void addScalar(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;

#if defined(HAVE_X86_SSE2)
  // If the inputVector address is not 16-byte aligned, the first several frames
  // (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector) && n) {
    *outputVector = *inputVector + scalar;
    inputVector++;
    outputVector++;
    n--;
  }

  // Now the inputVector address is aligned and start to apply SSE.
  size_t group = n / 4;
  __m128 mScalar = _mm_set_ps1(scalar);
  __m128 *pSource;
  __m128 *pDest;
  __m128 dest;

  bool destAligned = is16ByteAligned(outputVector);
  if (destAligned) { // all aligned
    while (group--) {
      pSource = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector));
      pDest = reinterpret_cast<__m128 *>(outputVector);
      *pDest = _mm_add_ps(*pSource, mScalar);

      inputVector += 4;
      outputVector += 4;
    }
  } else {
    while (group--) {
      pSource = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector));
      dest = _mm_add_ps(*pSource, mScalar);
      _mm_storeu_ps(outputVector, dest);

      inputVector += 4;
      outputVector += 4;
    }
  }

  // Non-SSE handling for remaining frames which is less than 4.
  n %= 4;
#elif defined(HAVE_ARM_NEON_INTRINSICS)
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;
  float32x4_t scalarVector = vdupq_n_f32(scalar);

  while (outputVector < endP) {
    float32x4_t source = vld1q_f32(inputVector);
    vst1q_f32(outputVector, vaddq_f32(source, scalarVector));

    inputVector += 4;
    outputVector += 4;
  }
  n = tailFrames;
#endif
  while (n--) {
    *outputVector = *inputVector + scalar;
    ++inputVector;
    ++outputVector;
  }
}

void add(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;

#if defined(HAVE_X86_SSE2)
  // If the inputVector address is not 16-byte aligned, the first several frames
  // (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector1) && n) {
    *outputVector = *inputVector1 + *inputVector2;
    inputVector1++;
    inputVector2++;
    outputVector++;
    n--;
  }

  // Now the inputVector1 address is aligned and start to apply SSE.
  size_t group = n / 4;
  __m128 *pSource1;
  __m128 *pSource2;
  __m128 *pDest;
  __m128 source2;
  __m128 dest;

  bool source2Aligned = is16ByteAligned(inputVector2);
  bool destAligned = is16ByteAligned(outputVector);

  if (source2Aligned && destAligned) { // all aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      pSource2 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector2));
      pDest = reinterpret_cast<__m128 *>(outputVector);
      *pDest = _mm_add_ps(*pSource1, *pSource2);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }

  } else if (source2Aligned && !destAligned) { // source2 aligned but dest not
                                               // aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      pSource2 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector2));
      dest = _mm_add_ps(*pSource1, *pSource2);
      _mm_storeu_ps(outputVector, dest);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }

  } else if (!source2Aligned && destAligned) { // source2 not aligned but dest
                                               // aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      source2 = _mm_loadu_ps(inputVector2);
      pDest = reinterpret_cast<__m128 *>(outputVector);
      *pDest = _mm_add_ps(*pSource1, source2);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }
  } else if (!source2Aligned && !destAligned) { // both source2 and dest not
                                                // aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      source2 = _mm_loadu_ps(inputVector2);
      dest = _mm_add_ps(*pSource1, source2);
      _mm_storeu_ps(outputVector, dest);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }
  }

  // Non-SSE handling for remaining frames which is less than 4.
  n %= 4;
#elif defined(HAVE_ARM_NEON_INTRINSICS)
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;

  while (outputVector < endP) {
    float32x4_t source1 = vld1q_f32(inputVector1);
    float32x4_t source2 = vld1q_f32(inputVector2);
    vst1q_f32(outputVector, vaddq_f32(source1, source2));

    inputVector1 += 4;
    inputVector2 += 4;
    outputVector += 4;
  }
  n = tailFrames;
#endif
  while (n--) {
    *outputVector = *inputVector1 + *inputVector2;
    ++inputVector1;
    ++inputVector2;
    ++outputVector;
  }
}

void subtract(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;

#if defined(HAVE_X86_SSE2)
  // If the inputVector address is not 16-byte aligned, the first several frames
  // (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector1) && n) {
    *outputVector = *inputVector1 - *inputVector2;
    inputVector1++;
    inputVector2++;
    outputVector++;
    n--;
  }

  // Now the inputVector1 address is aligned and start to apply SSE.
  size_t group = n / 4;
  __m128 *pSource1;
  __m128 *pSource2;
  __m128 *pDest;
  __m128 source2;
  __m128 dest;

  bool source2Aligned = is16ByteAligned(inputVector2);
  bool destAligned = is16ByteAligned(outputVector);

  if (source2Aligned && destAligned) { // all aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      pSource2 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector2));
      pDest = reinterpret_cast<__m128 *>(outputVector);
      *pDest = _mm_sub_ps(*pSource1, *pSource2);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }
  } else if (source2Aligned && !destAligned) { // source2 aligned but dest not
                                               // aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      pSource2 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector2));
      dest = _mm_sub_ps(*pSource1, *pSource2);
      _mm_storeu_ps(outputVector, dest);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }
  } else if (!source2Aligned && destAligned) { // source2 not aligned but dest
                                               // aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      source2 = _mm_loadu_ps(inputVector2);
      pDest = reinterpret_cast<__m128 *>(outputVector);
      *pDest = _mm_sub_ps(*pSource1, source2);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }
  } else if (!source2Aligned && !destAligned) { // both source2 and dest not
                                                // aligned
    while (group--) {
      pSource1 = reinterpret_cast<__m128 *>(const_cast<float *>(inputVector1));
      source2 = _mm_loadu_ps(inputVector2);
      dest = _mm_sub_ps(*pSource1, source2);
      _mm_storeu_ps(outputVector, dest);

      inputVector1 += 4;
      inputVector2 += 4;
      outputVector += 4;
    }
  }

  // Non-SSE handling for remaining frames which is less than 4.
  n %= 4;
#elif defined(HAVE_ARM_NEON_INTRINSICS)
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;

  while (outputVector < endP) {
    float32x4_t source1 = vld1q_f32(inputVector1);
    float32x4_t source2 = vld1q_f32(inputVector2);
    vst1q_f32(outputVector, vsubq_f32(source1, source2));

    inputVector1 += 4;
    inputVector2 += 4;
    outputVector += 4;
  }
  n = tailFrames;
#endif
  while (n--) {
    *outputVector = *inputVector1 - *inputVector2;
    ++inputVector1;
    ++inputVector2;
    ++outputVector;
  }
}

void multiply(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;

#if defined(HAVE_X86_SSE2)
  // If the inputVector1 address is not 16-byte aligned, the first several
  // frames (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector1) && n) {
    *outputVector = *inputVector1 * *inputVector2;
    inputVector1++;
    inputVector2++;
    outputVector++;
    n--;
  }

  // Now the inputVector1 address aligned and start to apply SSE.
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;
  __m128 pSource1;
  __m128 pSource2;
  __m128 dest;

  bool source2Aligned = is16ByteAligned(inputVector2);
  bool destAligned = is16ByteAligned(outputVector);

#define SSE2_MULT(loadInstr, storeInstr) \
  while (outputVector < endP) { \
    pSource1 = _mm_load_ps(inputVector1); \
    pSource2 = _mm_##loadInstr##_ps(inputVector2); \
    dest = _mm_mul_ps(pSource1, pSource2); \
    _mm_##storeInstr##_ps(outputVector, dest); \
    inputVector1 += 4; \
    inputVector2 += 4; \
    outputVector += 4; \
  }

  if (source2Aligned && destAligned) // Both aligned.
    SSE2_MULT(load, store)
  else if (source2Aligned && !destAligned) // Source2 is aligned but dest not.
    SSE2_MULT(load, storeu)
  else if (!source2Aligned && destAligned) // Dest is aligned but source2 not.
    SSE2_MULT(loadu, store)
  else // Neither aligned.
    SSE2_MULT(loadu, storeu)

  n = tailFrames;
#elif defined(HAVE_ARM_NEON_INTRINSICS)
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;

  while (outputVector < endP) {
    float32x4_t source1 = vld1q_f32(inputVector1);
    float32x4_t source2 = vld1q_f32(inputVector2);
    vst1q_f32(outputVector, vmulq_f32(source1, source2));

    inputVector1 += 4;
    inputVector2 += 4;
    outputVector += 4;
  }
  n = tailFrames;
#endif
  while (n--) {
    *outputVector = *inputVector1 * *inputVector2;
    ++inputVector1;
    ++inputVector2;
    ++outputVector;
  }
}

float maximumMagnitude(const float *inputVector, size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;
  float max = 0;

#if defined(HAVE_X86_SSE2)
  // If the inputVector address is not 16-byte aligned, the first several frames
  // (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector) && n) {
    max = std::max(max, std::abs(*inputVector));
    inputVector++;
    n--;
  }

  // Now the inputVector is aligned, use SSE.
  size_t tailFrames = n % 4;
  const float *endP = inputVector + n - tailFrames;
  __m128 source;
  __m128 mMax = _mm_setzero_ps();
  int mask = 0x7FFFFFFF;
  __m128 mMask = _mm_set1_ps(*reinterpret_cast<float *>(&mask));

  while (inputVector < endP) {
    source = _mm_load_ps(inputVector);
    // Calculate the absolute value by anding source with mask, the sign bit is
    // set to 0.
    source = _mm_and_ps(source, mMask);
    mMax = _mm_max_ps(mMax, source);
    inputVector += 4;
  }

  // Get max from the SSE results.
  const float *groupMaxP = reinterpret_cast<float *>(&mMax);
  max = std::max(max, groupMaxP[0]);
  max = std::max(max, groupMaxP[1]);
  max = std::max(max, groupMaxP[2]);
  max = std::max(max, groupMaxP[3]);

  n = tailFrames;
#elif defined(c)
  size_t tailFrames = n % 4;
  const float *endP = inputVector + n - tailFrames;

  float32x4_t fourMax = vdupq_n_f32(0);
  while (inputVector < endP) {
    float32x4_t source = vld1q_f32(inputVector);
    fourMax = vmaxq_f32(fourMax, vabsq_f32(source));
    inputVector += 4;
  }
  float32x2_t twoMax = vmax_f32(vget_low_f32(fourMax), vget_high_f32(fourMax));

  float groupMax[2];
  vst1_f32(groupMax, twoMax);
  max = std::max(groupMax[0], groupMax[1]);

  n = tailFrames;
#endif

  while (n--) {
    max = std::max(max, std::abs(*inputVector));
    ++inputVector;
  }

  return max;
}

void multiplyByScalarThenAddToOutput(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess) {
  size_t n = numberOfElementsToProcess;

#if HAVE_X86_SSE2
  // If the inputVector address is not 16-byte aligned, the first several frames
  // (at most three) should be processed separately.
  while (!is16ByteAligned(inputVector) && n) {
    *outputVector += scalar * *inputVector;
    inputVector++;
    outputVector++;
    n--;
  }

  // Now the inputVector is aligned, use SSE.
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;

  __m128 pSource;
  __m128 dest;
  __m128 temp;
  __m128 mScale = _mm_set_ps1(scalar);

  bool destAligned = is16ByteAligned(outputVector);

#define SSE2_MULT_ADD(loadInstr, storeInstr) \
  while (outputVector < endP) { \
    pSource = _mm_load_ps(inputVector); \
    temp = _mm_mul_ps(pSource, mScale); \
    dest = _mm_##loadInstr##_ps(outputVector); \
    dest = _mm_add_ps(dest, temp); \
    _mm_##storeInstr##_ps(outputVector, dest); \
    inputVector += 4; \
    outputVector += 4; \
  }

  if (destAligned)
    SSE2_MULT_ADD(load, store)
  else
    SSE2_MULT_ADD(loadu, storeu)

  n = tailFrames;
#elif HAVE_ARM_NEON_INTRINSICS
  size_t tailFrames = n % 4;
  const float *endP = outputVector + n - tailFrames;

  float32x4_t k = vdupq_n_f32(scalar);
  while (outputVector < endP) {
    float32x4_t source = vld1q_f32(inputVector);
    float32x4_t dest = vld1q_f32(outputVector);

    dest = vmlaq_f32(dest, source, k);
    vst1q_f32(outputVector, dest);

    inputVector += 4;
    outputVector += 4;
  }
  n = tailFrames;
#endif
  while (n--) {
    *outputVector += *inputVector * scalar;
    ++inputVector;
    ++outputVector;
  }
}

float computeConvolution(const float *state, const float *kernel, size_t kernelSize) {
    float sum = 0.0f;
    int k = 0;

#ifdef HAVE_ARM_NEON_INTRINSICS
    float32x4_t vSum = vdupq_n_f32(0.0f);

  // process 4 samples at a time
  for (; k <= kernelSize - 4; k += 4) {
    float32x4_t vState = vld1q_f32(state + k);
    float32x4_t vKernel = vld1q_f32(kernel + k);

    // fused multiply-add: vSum += vState * vKernel
    vSum = vmlaq_f32(vSum, vState, vKernel);
  }

  // horizontal reduction: Sum the 4 lanes of vSum into a single float
  sum += vgetq_lane_f32(vSum, 0);
  sum += vgetq_lane_f32(vSum, 1);
  sum += vgetq_lane_f32(vSum, 2);
  sum += vgetq_lane_f32(vSum, 3);
#endif
    for (; k < kernelSize; ++k) {
        sum += state[k] * kernel[k];
    }

    return sum;
}

void deinterleaveStereo(
        const float * __restrict inputInterleaved,
        float * __restrict outputLeft,
        float * __restrict outputRight,
        size_t numberOfFrames) {

    size_t n = numberOfFrames;

#if defined(HAVE_ARM_NEON_INTRINSICS)
    // process 4 frames (8 samples) at a time using NEON
    size_t group = n / 4;
    while (group--) {
        // vld2q_f32 deinterleaves L and R into separate registers in one hardware op
        float32x4x2_t v = vld2q_f32(inputInterleaved);
        vst1q_f32(outputLeft, v.val[0]);
        vst1q_f32(outputRight, v.val[1]);

        inputInterleaved += 8;
        outputLeft += 4;
        outputRight += 4;
    }
    n %= 4;
#elif defined(HAVE_X86_SSE2)
    // process 4 frames (8 samples) at a time using SSE
    size_t group = n / 4;
    while (group--) {
        // load two 128-bit registers (8 floats total)
        __m128 s0 = _mm_loadu_ps(inputInterleaved);
        __m128 s1 = _mm_loadu_ps(inputInterleaved + 4);

        // use shuffle to group the Left samples and Right samples
        // mask 0x88 (2,0,2,0) picks indices 0 and 2 from both s0 and s1
        // mask 0xDD (3,1,3,1) picks indices 1 and 3 from both s0 and s1
        __m128 left_v  = _mm_shuffle_ps(s0, s1, _MM_SHUFFLE(2, 0, 2, 0));
        __m128 right_v = _mm_shuffle_ps(s0, s1, _MM_SHUFFLE(3, 1, 3, 1));

        _mm_storeu_ps(outputLeft, left_v);
        _mm_storeu_ps(outputRight, right_v);

        inputInterleaved += 8;
        outputLeft += 4;
        outputRight += 4;
    }
    n %= 4;
#endif

    while (n--) {
        *outputLeft++ = *inputInterleaved++;
        *outputRight++ = *inputInterleaved++;
    }
}

void interleaveStereo(
        const float * __restrict inputLeft,
        const float * __restrict inputRight,
        float * __restrict outputInterleaved,
        size_t numberOfFrames) {

    size_t n = numberOfFrames;

#if defined(HAVE_ARM_NEON_INTRINSICS)
    // process 4 frames (8 samples) at a time
    size_t group = n / 4;
    while (group--) {
        // load contiguous planar data
        float32x4_t vL = vld1q_f32(inputLeft);
        float32x4_t vR = vld1q_f32(inputRight);

        // vst2q_f32 takes two registers and interleaves them during the store:
        float32x4x2_t vOut = { vL, vR };
        vst2q_f32(outputInterleaved, vOut);

        inputLeft += 4;
        inputRight += 4;
        outputInterleaved += 8;
    }
    n %= 4;
#elif defined(HAVE_X86_SSE2)
    // process 4 frames (8 samples) at a time
    size_t group = n / 4;
    while (group--) {
        __m128 vL = _mm_loadu_ps(inputLeft);
        __m128 vR = _mm_loadu_ps(inputRight);

        // unpack low: Interleaves first two elements of each register
        __m128 vLow = _mm_unpacklo_ps(vL, vR);

        // unpack high: Interleaves last two elements of each register
        __m128 vHigh = _mm_unpackhi_ps(vL, vR);

        _mm_storeu_ps(outputInterleaved, vLow);
        _mm_storeu_ps(outputInterleaved + 4, vHigh);

        inputLeft += 4;
        inputRight += 4;
        outputInterleaved += 8;
    }
    n %= 4;
#endif

    while (n--) {
        *outputInterleaved++ = *inputLeft++;
        *outputInterleaved++ = *inputRight++;
    }
}

#endif

} // namespace audioapi::dsp
