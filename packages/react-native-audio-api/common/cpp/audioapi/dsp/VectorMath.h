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

#pragma once

// Defines the interface for several vector math functions whose implementation
// will ideally be optimized.

#include <cstddef>

namespace audioapi::dsp {

void multiplyByScalarThenAddToOutput(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess);

void multiplyByScalar(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess);
void addScalar(
    const float *inputVector,
    float scalar,
    float *outputVector,
    size_t numberOfElementsToProcess);
void add(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess);
void subtract(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess);
void multiply(
    const float *inputVector1,
    const float *inputVector2,
    float *outputVector,
    size_t numberOfElementsToProcess);

// Finds the maximum magnitude of a float vector.
float maximumMagnitude(const float *inputVector, size_t numberOfElementsToProcess);

// Returns the dot product of two float vectors (sum of element-wise products).
float dotProduct(
    const float *inputVector1,
    const float *inputVector2,
    size_t numberOfElementsToProcess);

// Returns the sum of squares of a float vector
float sumOfSquares(const float *inputVector, size_t numberOfElementsToProcess);

void interleaveStereo(
    const float *inputLeft,
    const float *inputRight,
    float *outputInterleaved,
    size_t numberOfFrames);

void deinterleaveStereo(
    const float *inputInterleaved,
    float *outputLeft,
    float *outputRight,
    size_t numberOfFrames);

// Packs @p numberOfChannels planar channel pointers into one channel-interleaved buffer.
// @p outputInterleaved must hold numberOfFrames * numberOfChannels floats.
void interleave(
    const float *const *inputChannels,
    size_t numberOfChannels,
    float *outputInterleaved,
    size_t numberOfFrames);

// Splits a channel-interleaved buffer into @p numberOfChannels planar channel pointers,
// each of which must hold numberOfFrames floats.
void deinterleave(
    const float *inputInterleaved,
    float *const *outputChannels,
    size_t numberOfChannels,
    size_t numberOfFrames);

} // namespace audioapi::dsp
