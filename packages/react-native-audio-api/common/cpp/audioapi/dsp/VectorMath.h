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

#include <algorithm>
#include <cmath>
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

void linearToDecibels(
    const float *inputVector,
    float *outputVector,
    size_t numberOfElementsToProcess);

} // namespace audioapi::dsp
