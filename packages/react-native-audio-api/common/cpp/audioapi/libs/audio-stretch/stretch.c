////////////////////////////////////////////////////////////////////////////
//                        **** AUDIO-STRETCH ****                         //
//                      Time Domain Harmonic Scaler                       //
//                    Copyright (c) 2022 David Bryant                     //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// stretch.c

// Time Domain Harmonic Compression and Expansion
//
// This library performs time domain harmonic scaling with pitch detection
// to stretch the timing of a 16-bit PCM signal (either mono or stereo) from
// 1/2 to 2 times its original length. This is done without altering any of
// the tonal characteristics.
//
// Use stereo (num_chans = 2), when both channels are from same source
// and should contain approximately similar content.
// For independent channels, prefer using multiple StretchHandle-instances.
// see https://github.com/dbry/audio-stretch/issues/6


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "stretch.h"

#define MIN_PERIOD  24          /* minimum allowable pitch period */
#define MAX_PERIOD  2400        /* maximum allowable pitch period */

#if INT_MAX == 32767
#define MERGE_OFFSET    32768L      /* promote to long before offset */
#define abs32           labs        /* use long abs to avoid UB */
#else
#define MERGE_OFFSET    32768
#define abs32           abs
#endif

#define MAX_CORR    UINT32_MAX  /* maximum value for correlation ratios */

struct stretch_cnxt {
    int num_chans, inbuff_samples, shortest, longest, tail, head, fast_mode;
    int16_t *inbuff, *calcbuff;
    float outsamples_error;
    uint32_t *results;

    struct stretch_cnxt *next;
    int16_t *intermediate;
};

static void merge_blocks (int16_t *output, int16_t *input1, int16_t *input2, int samples);
static int find_period_fast (struct stretch_cnxt *cnxt, int16_t *samples);
static int find_period (struct stretch_cnxt *cnxt, int16_t *samples);

/*
 * Initialize a context of the time stretching code. The shortest and longest periods
 * are specified here. The longest period determines the lowest fundamental frequency
 * that can be handled correctly. Note that higher frequencies can be handled than the
 * shortest period would suggest because multiple periods can be combined, and the
 * worst-case performance will suffer if too short a period is selected. The flags are:
 *
 * STRETCH_FAST_FLAG    0x1     Use the "fast" version of the period calculation
 *
 * STRETCH_DUAL_FLAG    0x2     Cascade two instances of the stretcher to expand
 *                              available ratios to 0.25X to 4.00X
 */

StretchHandle stretch_init (int shortest_period, int longest_period, int num_channels, int flags)
{
    struct stretch_cnxt *cnxt;
    int max_periods = 3;

    if (flags & STRETCH_FAST_FLAG) {
        longest_period = (longest_period + 1) & ~1;
        shortest_period &= ~1;
        max_periods = 4;
    }

    if (longest_period <= shortest_period || shortest_period < MIN_PERIOD || longest_period > MAX_PERIOD) {
        fprintf (stderr, "stretch_init(): invalid periods!\n");
        return NULL;
    }

    cnxt = (struct stretch_cnxt *) calloc (1, sizeof (struct stretch_cnxt));

    if (cnxt) {
        cnxt->inbuff_samples = longest_period * num_channels * max_periods;
        cnxt->inbuff = calloc (cnxt->inbuff_samples, sizeof (*cnxt->inbuff));

        if (num_channels == 2 || (flags & STRETCH_FAST_FLAG))
            cnxt->calcbuff = calloc (longest_period * num_channels, sizeof (*cnxt->calcbuff));

        if ((flags & STRETCH_FAST_FLAG))
            cnxt->results = calloc (longest_period, sizeof (*cnxt->results));
    }

    if (!cnxt || !cnxt->inbuff || (num_channels == 2 && (flags & STRETCH_FAST_FLAG) && !cnxt->calcbuff) || ((flags & STRETCH_FAST_FLAG) && !cnxt->results)) {
        fprintf (stderr, "stretch_init(): out of memory!\n");
        return NULL;
    }

    cnxt->head = cnxt->tail = cnxt->longest = longest_period * num_channels;
    cnxt->fast_mode = (flags & STRETCH_FAST_FLAG) ? 1 : 0;
    cnxt->shortest = shortest_period * num_channels;
    cnxt->num_chans = num_channels;

    if (flags & STRETCH_DUAL_FLAG) {
        cnxt->next = stretch_init (shortest_period, longest_period, num_channels, flags & ~STRETCH_DUAL_FLAG);
        cnxt->intermediate = calloc (longest_period * num_channels * max_periods, sizeof (*cnxt->intermediate));
    }

    return (StretchHandle) cnxt;
}

/*
 * Re-Initialize a context of the time stretching code - as if freshly created
 * with stretch_init(). This drops all internal state.
 */

void stretch_reset (StretchHandle handle)
{
    struct stretch_cnxt *cnxt = (struct stretch_cnxt *) handle;

    cnxt->head = cnxt->tail = cnxt->longest;
    memset (cnxt->inbuff, 0, cnxt->tail * sizeof (*cnxt->inbuff));

    if (cnxt->next)
        stretch_reset (cnxt->next);
}

/*
 * Determine how many samples (per channel) should be reserved in 'output'-array
 * for stretch_samples() and stretch_flush(). max_num_samples and max_ratio are the
 * maximum values that will be passed to stretch_samples().
 */

int stretch_output_capacity (StretchHandle handle, int max_num_samples, float max_ratio)
{
    struct stretch_cnxt *cnxt = (struct stretch_cnxt *) handle;
    int max_period = cnxt->longest / cnxt->num_chans;
    int max_expected_samples;
    float next_ratio;

    if (cnxt->next) {
        if (max_ratio < 0.5) {
            next_ratio = max_ratio / 0.5;
            max_ratio = 0.5;
        }
        else if (max_ratio > 2.0) {
            next_ratio = max_ratio / 2.0;
            max_ratio = 2.0;
        }
        else
            next_ratio = 1.0;
    }

    max_expected_samples = (int) ceil (max_num_samples * ceil (max_ratio * 2.0) / 2.0) +
        max_period * (cnxt->fast_mode ? 4 : 3);

    if (cnxt->next)
        max_expected_samples = stretch_output_capacity (cnxt->next, max_expected_samples, next_ratio);

    return max_expected_samples;
}

/*
 * Process the specified samples with the given ratio (which is normally clipped to
 * the range 0.5 to 2.0, or 0.25 to 4.00 for the "dual" mode). Note that in stereo
 * the number of samples refers to the samples for one channel (i.e., not the total
 * number of values passed) and can be as large as desired (samples are buffered here).
 * The ratio may change between calls, but there is some latency to consider because
 * audio is buffered here and a new ratio may be applied to previously sent samples.
 *
 * The exact number of samples output is not easy to determine in advance, so a function
 * is provided (stretch_output_capacity()) that calculates the maximum number of samples
 * that can be generated from a single call to this function (or stretch_flush()) given
 * a number of samples and maximum ratio. It is reccomended that that function be used
 * after initialization to allocate in advance the buffer size required. Be sure to
 * multiply the return value by the number channels!
 */

int stretch_samples (StretchHandle handle, const int16_t *samples, int num_samples, int16_t *output, float ratio)
{
    struct stretch_cnxt *cnxt = (struct stretch_cnxt *) handle;
    int out_samples = 0, next_samples = 0;
    int16_t *outbuf = output;
    float next_ratio;

    /* if there's a cascaded instance after this one, try to do as much of the ratio here and the rest in "next" */

    if (cnxt->next) {
        outbuf = cnxt->intermediate;

        if (ratio < 0.5) {
            next_ratio = ratio / 0.5;
            ratio = 0.5;
        }
        else if (ratio > 2.0) {
            next_ratio = ratio / 2.0;
            ratio = 2.0;
        }
        else
            next_ratio = 1.0;
    }

    num_samples *= cnxt->num_chans;

    /* this really should not happen, but a good idea to clamp in case */

    if (ratio < 0.5)
        ratio = 0.5;
    else if (ratio > 2.0)
        ratio = 2.0;

    /* while we have pending samples to read into our buffer */

    while (num_samples) {

        /* copy in as many samples as we have room for */

        int samples_to_copy = num_samples;

        if (samples_to_copy > cnxt->inbuff_samples - cnxt->head)
            samples_to_copy = cnxt->inbuff_samples - cnxt->head;

        memcpy (cnxt->inbuff + cnxt->head, samples, samples_to_copy * sizeof (cnxt->inbuff [0]));
        num_samples -= samples_to_copy;
        samples += samples_to_copy;
        cnxt->head += samples_to_copy;

        /* while there are enough samples to process (3 or 4 times the longest period), do so */

        while (cnxt->tail >= cnxt->longest && cnxt->head - cnxt->tail >= cnxt->longest * (cnxt->fast_mode ? 3 : 2)) {
            float process_ratio;
            int period;

            if (ratio != 1.0 || cnxt->outsamples_error)
                period = cnxt->fast_mode ? find_period_fast (cnxt, cnxt->inbuff + cnxt->tail) :
                    find_period (cnxt, cnxt->inbuff + cnxt->tail);
            else
                period = cnxt->longest;

            /*
             * Once we have calculated the best-match period, there are 4 possible transformations
             * available to convert the input samples to output samples. Obviously we can simply
             * copy the samples verbatim (1:1). Standard TDHS provides algorithms for 2:1 and
             * 1:2 scaling, and I have created an obvious extension for 2:3 scaling. To achieve
             * intermediate ratios we maintain a "error" term (in samples) and use that here to
             * calculate the actual transformation to apply.
             */

            if (cnxt->outsamples_error == 0.0)
                process_ratio = floor (ratio * 2.0 + 0.5) / 2.0;
            else if (cnxt->outsamples_error > 0.0)
                process_ratio = floor (ratio * 2.0) / 2.0;
            else
                process_ratio = ceil (ratio * 2.0) / 2.0;

            if (process_ratio == 0.5) {
                merge_blocks (outbuf + out_samples, cnxt->inbuff + cnxt->tail,
                    cnxt->inbuff + cnxt->tail + period, period);
                cnxt->outsamples_error += period - (period * 2.0 * ratio);
                out_samples += period;
                cnxt->tail += period * 2;
            }
            else if (process_ratio == 1.0) {
                memcpy (outbuf + out_samples, cnxt->inbuff + cnxt->tail, period * 2 * sizeof (cnxt->inbuff [0]));

                if (ratio != 1.0)
                    cnxt->outsamples_error += (period * 2.0) - (period * 2.0 * ratio);
                else
                    cnxt->outsamples_error = 0; /* if the ratio is 1.0, we can never cancel the error, so just do it now */

                out_samples += period * 2;
                cnxt->tail += period * 2;
            }
            else if (process_ratio == 1.5) {
                memcpy (outbuf + out_samples, cnxt->inbuff + cnxt->tail, period * sizeof (cnxt->inbuff [0]));
                merge_blocks (outbuf + out_samples + period, cnxt->inbuff + cnxt->tail + period,
                    cnxt->inbuff + cnxt->tail, period);
                memcpy (outbuf + out_samples + period * 2, cnxt->inbuff + cnxt->tail + period, period * sizeof (cnxt->inbuff [0]));
                cnxt->outsamples_error += (period * 3.0) - (period * 2.0 * ratio);
                out_samples += period * 3;
                cnxt->tail += period * 2;
            }
            else if (process_ratio == 2.0) {
                merge_blocks (outbuf + out_samples, cnxt->inbuff + cnxt->tail,
                    cnxt->inbuff + cnxt->tail - period, period * 2);

                cnxt->outsamples_error += (period * 2.0) - (period * ratio);
                out_samples += period * 2;
                cnxt->tail += period;

                if (cnxt->fast_mode) {
                    merge_blocks (outbuf + out_samples, cnxt->inbuff + cnxt->tail,
                        cnxt->inbuff + cnxt->tail - period, period * 2);

                    cnxt->outsamples_error += (period * 2.0) - (period * ratio);
                    out_samples += period * 2;
                    cnxt->tail += period;
                }
            }
            else
                fprintf (stderr, "stretch_samples: fatal programming error: process_ratio == %g\n", process_ratio);

            /* if there's another cascaded instance after this, pass the just stretched samples into that */

            if (cnxt->next) {
                next_samples += stretch_samples (cnxt->next, outbuf, out_samples / cnxt->num_chans, output + next_samples * cnxt->num_chans, next_ratio);
                out_samples = 0;
            }

            /* finally, left-justify the samples in the buffer leaving one longest period of history */

            int samples_to_move = cnxt->inbuff_samples - cnxt->tail + cnxt->longest;

            memmove (cnxt->inbuff, cnxt->inbuff + cnxt->tail - cnxt->longest,
                samples_to_move * sizeof (cnxt->inbuff [0]));

            cnxt->head -= cnxt->tail - cnxt->longest;
            cnxt->tail = cnxt->longest;
        }
    }

    /*
     * This code is not strictly required, but will reduce latency, especially in the dual-instance case, by
     * always flushing all pending samples if no actual stretching is desired (i.e., ratio is 1.0 and there's
     * no error to compensate for). This case is more common now than previously because of the gap detection
     * and cascaded instances.
     */

    if (ratio == 1.0 && !cnxt->outsamples_error && cnxt->head != cnxt->tail) {
        int samples_leftover = cnxt->head - cnxt->tail;

        if (cnxt->next)
            next_samples += stretch_samples (cnxt->next, cnxt->inbuff + cnxt->tail, samples_leftover / cnxt->num_chans,
                output + next_samples * cnxt->num_chans, next_ratio);
        else {
            memcpy (outbuf + out_samples, cnxt->inbuff + cnxt->tail, samples_leftover * sizeof (*output));
            out_samples += samples_leftover;
        }

        memmove (cnxt->inbuff, cnxt->inbuff + cnxt->head - cnxt->longest, cnxt->longest * sizeof (cnxt->inbuff [0]));
        cnxt->head = cnxt->tail = cnxt->longest;
    }

    return cnxt->next ? next_samples : out_samples / cnxt->num_chans;
}  

/*
 * Flush any leftover samples out at normal speed. For cascaded dual instances this must be called
 * twice to completely flush, or simply call it until it returns zero samples. The maximum number
 * of samples that can be returned from each call of this function can be determined in advance with
 * stretch_output_capacity().
 */

int stretch_flush (StretchHandle handle, int16_t *output)
{
    struct stretch_cnxt *cnxt = (struct stretch_cnxt *) handle;
    int samples_leftover = cnxt->head - cnxt->tail;
    int samples_flushed = 0;

    if (cnxt->next) {
        if (samples_leftover)
            samples_flushed = stretch_samples (cnxt->next, cnxt->inbuff + cnxt->tail, samples_leftover / cnxt->num_chans, output, 1.0);

        if (!samples_flushed)
            samples_flushed = stretch_flush (cnxt->next, output);
    }
    else {
        memcpy (output, cnxt->inbuff + cnxt->tail, samples_leftover * sizeof (*output));
        samples_flushed = samples_leftover / cnxt->num_chans;
    }

    cnxt->tail = cnxt->head;
    memset (cnxt->inbuff, 0, cnxt->tail * sizeof (*cnxt->inbuff));

    return samples_flushed;
}

/* free handle */

void stretch_deinit (StretchHandle handle)
{
    struct stretch_cnxt *cnxt = (struct stretch_cnxt *) handle;

    free (cnxt->calcbuff);
    free (cnxt->results);
    free (cnxt->inbuff);

    if (cnxt->next) {
        stretch_deinit (cnxt->next);
        free (cnxt->intermediate);
    }

    free (cnxt);
}

/*
 * The pitch detection is done by finding the period that produces the
 * maximum value for the following correlation formula applied to two
 * consecutive blocks of the given period length:
 *
 *         sum of the absolute values of each sample in both blocks
 *   ---------------------------------------------------------------------
 *   sum of the absolute differences of each corresponding pair of samples
 *
 * This formula was chosen for two reasons.  First, it produces output values
 * that can directly compared regardless of the pitch period.  Second, the
 * numerator can be accumulated for successive periods, and only the
 * denominator need be completely recalculated.
 */

static int find_period (struct stretch_cnxt *cnxt, int16_t *samples)
{
    uint32_t sum, diff, factor, scaler, best_factor = 0;
    int16_t *calcbuff = samples;
    int period, best_period;
    int i, j;

    period = best_period = cnxt->shortest / cnxt->num_chans;

    // convert stereo to mono, and accumulate sum for longest period

    if (cnxt->num_chans == 2) {
        calcbuff = cnxt->calcbuff;

        for (sum = i = j = 0; i < cnxt->longest * 2; i += 2)
            sum += abs32 (calcbuff [j++] = ((int32_t) samples [i] + samples [i+1]) >> 1);
    }
    else
        for (sum = i = 0; i < cnxt->longest; ++i)
            sum += abs32 (calcbuff [i]) + abs32 (calcbuff [i+cnxt->longest]);

    // if silence return longest period, else calculate scaler based on largest sum

    if (sum)
        scaler = (MAX_CORR - 1) / sum;
    else
        return cnxt->longest;

    /* accumulate sum for shortest period size */

    for (sum = i = 0; i < period; ++i)
        sum += abs32 (calcbuff [i]) + abs32 (calcbuff [i+period]);

    /* this loop actually cycles through all period lengths */

    while (1) {
        int16_t *comp = calcbuff + period * 2;
        int16_t *ref = calcbuff + period;

        /* compute sum of absolute differences */

        diff = 0;

        while (ref != calcbuff)
            diff += abs32 ((int32_t) *--ref - *--comp);

        /*
         * Here we calculate and store the resulting correlation
         * factor.  Note that we must watch for a difference of
         * zero, meaning a perfect match.  Also, for increased
         * precision using integer math, we scale the sum.
         */

        factor = diff ? (sum * scaler) / diff : MAX_CORR;

        if (factor >= best_factor) {
            best_factor = factor;
            best_period = period;
        }

        /* see if we're done */

        if (period * cnxt->num_chans == cnxt->longest)
            break;

        /* update accumulating sum and current period */

        sum += abs32 (calcbuff [period * 2]) + abs32 (calcbuff [period * 2 + 1]);
        period++;
    }

    return best_period * cnxt->num_chans;
}

/*
 * This pitch detection function is similar to find_period() above, except that it
 * is optimized for speed. The audio data corresponding to two maximum periods is
 * averaged 2:1 into the calculation buffer, and then the calulations are done
 * for every other period length. Because the time is essentially proportional to
 * both the number of samples and the number of period lengths to try, this scheme
 * can reduce the time by a factor approaching 4x. The correlation results on either
 * side of the peak are compared to calculate a more accurate center of the period.
 */

static int find_period_fast (struct stretch_cnxt *cnxt, int16_t *samples)
{
    uint32_t sum, diff, scaler, best_factor = 0;
    int period, best_period;
    int i, j;

    best_period = period = cnxt->shortest / (cnxt->num_chans * 2);

    /* first step is compressing data 2:1 into calcbuff, and calculating maximum sum */

    if (cnxt->num_chans == 2)
        for (sum = i = j = 0; i < cnxt->longest * 2; i += 4)
            sum += abs32 (cnxt->calcbuff [j++] = ((int32_t) samples [i] + samples [i+1] + samples [i+2] + samples [i+3]) >> 2);
    else
        for (sum = i = j = 0; i < cnxt->longest * 2; i += 2)
            sum += abs32 (cnxt->calcbuff [j++] = ((int32_t) samples [i] + samples [i+1]) >> 1);

    // if silence return longest period, else calculate scaler based on largest sum

    if (sum)
        scaler = (MAX_CORR - 1) / sum;
    else
        return cnxt->longest;

    /* accumulate sum for shortest period */

    for (sum = i = 0; i < period; ++i)
        sum += abs32 (cnxt->calcbuff [i]) + abs32 (cnxt->calcbuff [i+period]);

    /* this loop actually cycles through all period lengths */

    while (1) {
        int16_t *comp = cnxt->calcbuff + period * 2;
        int16_t *ref = cnxt->calcbuff + period;

        /* compute sum of absolute differences */

        diff = 0;

        while (ref != cnxt->calcbuff)
            diff += abs32 ((int32_t) *--ref - *--comp);

        /*
         * Here we calculate and store the resulting correlation
         * factor.  Note that we must watch for a difference of
         * zero, meaning a perfect match.  Also, for increased
         * precision using integer math, we scale the sum.
         */

        cnxt->results [period] = diff ? (sum * scaler) / diff : MAX_CORR;

        if (cnxt->results [period] >= best_factor) {    /* check if best yet */
            best_factor = cnxt->results [period];
            best_period = period;
        }

        /* see if we're done */

        if (period * cnxt->num_chans * 2 == cnxt->longest)
            break;

        /* update accumulating sum and current period */

        sum += abs32 (cnxt->calcbuff [period * 2]) + abs32 (cnxt->calcbuff [period * 2 + 1]);
        period++;
    }

    if (best_period * cnxt->num_chans * 2 != cnxt->shortest && best_period * cnxt->num_chans * 2 != cnxt->longest) {
        uint32_t high_side_diff = cnxt->results [best_period] - cnxt->results [best_period+1];
        uint32_t low_side_diff = cnxt->results [best_period] - cnxt->results [best_period-1];

        if ((low_side_diff + 1) / 2 > high_side_diff)
            best_period = best_period * 2 + 1;
        else if ((high_side_diff + 1) / 2 > low_side_diff)
            best_period = best_period * 2 - 1;
        else
            best_period *= 2;
    }
    else
        best_period *= 2;           /* shortest or longest use as is */

    return best_period * cnxt->num_chans;
}

/*
 * To combine the two periods into one, each corresponding pair of samples
 * are averaged with a linearly sliding scale.  At the beginning of the period
 * the first sample dominates, and at the end the second sample dominates.  In
 * this way the resulting block blends with the previous and next blocks.
 *
 * The signed values are offset to unsigned for the calculation and then offset
 * back to signed.  This is done to avoid the compression around zero that occurs
 * with calculations of this type on C implementations that round division toward
 * zero.
 *
 * The maximum period handled here without overflow possibility is 65535 samples.
 * This corresponds to a maximum calculated period of 16383 samples (2x for stereo
 * and 2x for the "2.0" version of the stretch algorithm). Since the maximum
 * calculated period is currently set for 2400 samples, we have plenty of margin.
 */

static void merge_blocks (int16_t *output, int16_t *input1, int16_t *input2, int samples)
{
    int i;

    for (i = 0; i < samples; ++i)
        output [i] = (int32_t)(((uint32_t)(input1 [i] + MERGE_OFFSET) * (samples - i) +
            (uint32_t)(input2 [i] + MERGE_OFFSET) * i) / samples) - MERGE_OFFSET;
}
