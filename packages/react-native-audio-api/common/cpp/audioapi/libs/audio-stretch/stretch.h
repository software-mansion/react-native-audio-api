////////////////////////////////////////////////////////////////////////////
//                        **** AUDIO-STRETCH ****                         //
//                      Time Domain Harmonic Scaler                       //
//                    Copyright (c) 2022 David Bryant                     //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// stretch.h

// Time Domain Harmonic Compression and Expansion
//
// This library performs time domain harmonic scaling with pitch detection
// to stretch the timing of a 16-bit PCM signal (either mono or stereo) from
// 1/2 to 2 times its original length. This is done without altering any of
// its tonal characteristics.
//
// Use stereo (num_chans = 2), when both channels are from same source
// and should contain approximately similar content.
// For independent channels, prefer using multiple StretchHandle-instances.
// see https://github.com/dbry/audio-stretch/issues/6

#ifndef STRETCH_H
#define STRETCH_H

#include <stdint.h>

#define STRETCH_FAST_FLAG    0x1    // use "fast" version of period determination code
#define STRETCH_DUAL_FLAG    0x2    // cascade two instances (doubles usable ratio range)

#ifdef __cplusplus
extern "C" {
#endif

typedef void *StretchHandle;

StretchHandle stretch_init (int shortest_period, int longest_period, int num_chans, int flags);
int stretch_output_capacity (StretchHandle handle, int max_num_samples, float max_ratio);
int stretch_samples (StretchHandle handle, const int16_t *samples, int num_samples, int16_t *output, float ratio);
int stretch_flush (StretchHandle handle, int16_t *output);
void stretch_reset (StretchHandle handle);
void stretch_deinit (StretchHandle handle);

#ifdef __cplusplus
}
#endif

#endif

