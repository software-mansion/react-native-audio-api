import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { ScrollView } from 'react-native-gesture-handler';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors } from '../../styles';
import voiceAsset from '../AudioFile/voice-sample-landing.mp3';
import track3Asset from '../../demos/Crossfade/tracks/track3.mp3';

/** Max content length of each half (seconds); actual half may be shorter. */
const TARGET_HALF_SECONDS = 5;
/** Delayed start for the first source (seconds). */
const START_DELAY_SECONDS = 1;
/** Synthetic DC buffer long enough for two halves. */
const DC_ONES_DURATION_SECONDS = TARGET_HALF_SECONDS * 2;
const MIN_PLAYBACK_RATE = 0.5;
const MAX_PLAYBACK_RATE = 5;
const PLAYBACK_RATE_STEP = 0.1;
/** Wall-clock silence inserted at the cut so you can hear where it is. */
const CUT_SILENCE_PAUSE_SECONDS = 0.15;
/** RMS window used to detect “lady speaking” / high energy. */
const ENERGY_WINDOW_SECONDS = 0.03;
/** Step when scanning for the loudest cut near mid-file. */
const ENERGY_STEP_SECONDS = 0.01;
/** Min gap between alternate speech-cut candidates. */
const MIN_PEAK_SEPARATION_SECONDS = 2.0;
/** How many distinct speech-cut fragments to offer. */
const MAX_SPEECH_FRAGMENTS = 3;

type SampleAsset = 'voice' | 'track3' | 'dcOnes';

const SAMPLE_ASSETS: Partial<Record<SampleAsset, number>> = {
  voice: voiceAsset,
  track3: track3Asset,
};

const SAMPLE_LABELS: Record<SampleAsset, string> = {
  voice: 'Voice',
  track3: 'Music (track3)',
  dcOnes: 'DC ones (1.0)',
};

/** Equal halves around a high-energy cut. */
type SplitPlan = {
  /** Absolute cut time in the original buffer (seconds). */
  cutSeconds: number;
  /** Content length of each half (seconds). */
  halfSeconds: number;
  /** Start of the pruned region in the original buffer. */
  regionStartSeconds: number;
  /** Total pruned content length (= 2 × halfSeconds). */
  regionDurationSeconds: number;
  /** RMS at the chosen cut (speech / high-wave score). */
  cutRms: number;
  /** 1-based fragment index among detected speech peaks. */
  fragmentIndex: number;
};

type LoadedSample = {
  key: SampleAsset;
  sourceId: number | 'dcOnes';
  buffer: AudioBuffer;
  plans: SplitPlan[];
};

/** Constant-amplitude buffer for fair WSOLA cold-start measurement (no leading silence). */
function createDcOnesBuffer(
  context: AudioContext,
  durationSeconds: number
): AudioBuffer {
  const sampleRate = context.sampleRate;
  const length = Math.max(1, Math.floor(durationSeconds * sampleRate));
  const buffer = context.createBuffer(1, length, sampleRate);
  buffer.getChannelData(0).fill(1);
  return buffer;
}

function sliceAudioBuffer(
  context: AudioContext,
  source: AudioBuffer,
  startSeconds: number,
  durationSeconds: number
): AudioBuffer {
  const sampleRate = source.sampleRate;
  const startFrame = Math.floor(startSeconds * sampleRate);
  const frameCount = Math.min(
    Math.floor(durationSeconds * sampleRate),
    source.length - startFrame
  );

  const sliced = context.createBuffer(
    source.numberOfChannels,
    frameCount,
    sampleRate
  );

  for (let channel = 0; channel < source.numberOfChannels; channel += 1) {
    const channelData = source.getChannelData(channel);
    // Do not use copyToChannel(subarray(...)): native copyToChannel reads the
    // underlying ArrayBuffer from index 0 and ignores the TypedArray view offset,
    // so every slice would become the first half.
    sliced
      .getChannelData(channel)
      .set(channelData.subarray(startFrame, startFrame + frameCount));
  }

  return sliced;
}

function windowRms(
  channel: Float32Array,
  centerFrame: number,
  windowFrames: number
): number {
  const half = Math.floor(windowFrames / 2);
  const from = Math.max(0, centerFrame - half);
  const to = Math.min(channel.length, from + windowFrames);
  const count = to - from;
  if (count <= 0) {
    return 0;
  }
  let sumSq = 0;
  for (let i = from; i < to; i += 1) {
    const s = channel[i]!;
    sumSq += s * s;
  }
  return Math.sqrt(sumSq / count);
}

/**
 * Find up to MAX_SPEECH_FRAGMENTS distinct high-energy cuts across the file
 * (usable range leaves room for equal halves ≤ targetHalfSeconds), then prune
 * equal halves around each cut.
 */
function findSpeechSplitPlans(
  buffer: AudioBuffer,
  targetHalfSeconds = TARGET_HALF_SECONDS
): SplitPlan[] {
  const { sampleRate, length, duration } = buffer;
  const channel = buffer.getChannelData(0);
  const windowFrames = Math.max(1, Math.floor(sampleRate * ENERGY_WINDOW_SECONDS));
  const stepFrames = Math.max(1, Math.floor(sampleRate * ENERGY_STEP_SECONDS));
  const minSeparationFrames = Math.floor(
    sampleRate * MIN_PEAK_SEPARATION_SECONDS
  );

  // Cuts need room for equal halves on both sides.
  const halfBudget = Math.min(targetHalfSeconds, duration / 2);
  const minCutFrame = Math.max(windowFrames, Math.floor(sampleRate * halfBudget));
  const maxCutFrame = Math.min(
    length - windowFrames,
    Math.floor(sampleRate * (duration - halfBudget))
  );

  if (maxCutFrame <= minCutFrame) {
    const cutSeconds = duration / 2;
    const halfSeconds = Math.min(targetHalfSeconds, cutSeconds, duration - cutSeconds);
    return [
      {
        cutSeconds,
        halfSeconds,
        regionStartSeconds: cutSeconds - halfSeconds,
        regionDurationSeconds: halfSeconds * 2,
        cutRms: windowRms(channel, Math.floor(cutSeconds * sampleRate), windowFrames),
        fragmentIndex: 1,
      },
    ];
  }

  type Peak = { frame: number; rms: number };
  const samples: Peak[] = [];
  for (let frame = minCutFrame; frame <= maxCutFrame; frame += stepFrames) {
    samples.push({ frame, rms: windowRms(channel, frame, windowFrames) });
  }

  // Local maxima only (strictly louder than neighbors).
  const localMaxima: Peak[] = [];
  for (let i = 1; i < samples.length - 1; i += 1) {
    const prev = samples[i - 1]!;
    const cur = samples[i]!;
    const next = samples[i + 1]!;
    if (cur.rms >= prev.rms && cur.rms > next.rms) {
      localMaxima.push(cur);
    }
  }
  if (localMaxima.length === 0 && samples.length > 0) {
    localMaxima.push(
      samples.reduce((best, p) => (p.rms > best.rms ? p : best), samples[0]!)
    );
  }

  localMaxima.sort((a, b) => b.rms - a.rms);

  const selected: Peak[] = [];
  for (const peak of localMaxima) {
    if (selected.length >= MAX_SPEECH_FRAGMENTS) {
      break;
    }
    const tooClose = selected.some(
      (other) => Math.abs(other.frame - peak.frame) < minSeparationFrames
    );
    if (!tooClose) {
      selected.push(peak);
    }
  }

  // Stable UI order: earlier → later in the file.
  selected.sort((a, b) => a.frame - b.frame);

  return selected.map((peak, index) => {
    const cutSeconds = peak.frame / sampleRate;
    const halfSeconds = Math.min(
      targetHalfSeconds,
      cutSeconds,
      duration - cutSeconds
    );
    return {
      cutSeconds,
      halfSeconds,
      regionStartSeconds: cutSeconds - halfSeconds,
      regionDurationSeconds: halfSeconds * 2,
      cutRms: peak.rms,
      fragmentIndex: index + 1,
    };
  });
}

type RunMode = 'glued' | 'unbroken';

/**
 * Experiment: detect several high-energy speech peaks, prune equal halves
 * around the selected cut, then chain two AudioBufferSourceNodes so the second
 * starts when the first should end (wall = half / rate). Listen at the cut to
 * judge glue quality.
 */
const WsolaScheduleSplit: FC = () => {
  const [isLoading, setIsLoading] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [sample, setSample] = useState<SampleAsset>('voice');
  const [playbackRate, setPlaybackRate] = useState(2);
  const [silencePauseEnabled, setSilencePauseEnabled] = useState(true);
  const [fragmentIndex, setFragmentIndex] = useState(0);
  const [plans, setPlans] = useState<SplitPlan[]>([]);
  const [status, setStatus] = useState('Idle');
  const contextRef = useRef<AudioContext | null>(null);
  const fullBufferRef = useRef<LoadedSample | null>(null);
  const sourcesRef = useRef<AudioBufferSourceNode[]>([]);

  const plan = plans[fragmentIndex] ?? null;

  const getContext = useCallback(() => {
    if (!contextRef.current) {
      contextRef.current = new AudioContext();
    }
    return contextRef.current;
  }, []);

  const stopSources = useCallback(() => {
    for (const source of sourcesRef.current) {
      try {
        source.onEnded = null;
        source.stop();
      } catch {
        // already stopped
      }
      // Finished sources must leave the graph, otherwise they stay wired to the
      // destination and keep being mixed after playback ends.
      try {
        source.disconnect();
      } catch {
        // already disconnected
      }
    }
    sourcesRef.current = [];
    setIsRunning(false);
  }, []);

  const describePlans = useCallback((key: SampleAsset, nextPlans: SplitPlan[], duration: number) => {
    return [
      `sample=${key} loaded ${duration.toFixed(2)}s · ${nextPlans.length} speech fragment(s)`,
      ...nextPlans.map(
        (p) =>
          `  #${p.fragmentIndex} cut @ ${p.cutSeconds.toFixed(3)}s rms=${p.cutRms.toFixed(3)} · 2×${p.halfSeconds.toFixed(2)}s`
      ),
    ].join('\n');
  }, []);

  const loadFullBuffer = useCallback(
    async (key: SampleAsset) => {
      const sourceId = key === 'dcOnes' ? 'dcOnes' : SAMPLE_ASSETS[key]!;
      if (
        fullBufferRef.current?.key === key &&
        fullBufferRef.current.sourceId === sourceId
      ) {
        setPlans(fullBufferRef.current.plans);
        return fullBufferRef.current;
      }

      const context = getContext();
      const buffer =
        key === 'dcOnes'
          ? createDcOnesBuffer(context, DC_ONES_DURATION_SECONDS)
          : await context.decodeAudioData(sourceId as number);
      const nextPlans = findSpeechSplitPlans(buffer);
      const loaded: LoadedSample = { key, sourceId, buffer, plans: nextPlans };
      fullBufferRef.current = loaded;
      setPlans(nextPlans);
      setFragmentIndex(0);
      return loaded;
    },
    [getContext]
  );

  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const loaded = await loadFullBuffer(sample);
        if (!cancelled) {
          setPlans(loaded.plans);
          setFragmentIndex(0);
          setStatus(
            describePlans(sample, loaded.plans, loaded.buffer.duration)
          );
        }
      } catch (error) {
        if (!cancelled) {
          setStatus(`Load error: ${String(error)}`);
        }
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [describePlans, loadFullBuffer, sample]);

  const selectFragment = useCallback(
    (index: number) => {
      const next = plans[index];
      if (!next) {
        return;
      }
      setFragmentIndex(index);
      setStatus(
        [
          `selected fragment #${next.fragmentIndex}`,
          `speech cut @ ${next.cutSeconds.toFixed(3)}s (rms=${next.cutRms.toFixed(3)})`,
          `pruned ${next.regionDurationSeconds.toFixed(2)}s = 2×${next.halfSeconds.toFixed(2)}s around cut`,
          `region [${next.regionStartSeconds.toFixed(3)}, ${(
            next.regionStartSeconds + next.regionDurationSeconds
          ).toFixed(3)})s`,
        ].join('\n')
      );
    },
    [plans]
  );
  const runExperiment = useCallback(
    async (mode: RunMode) => {
      const rate = playbackRate;
      // Pitch correction only when stretching; rate 1 is the plain path.
      const pitchCorrection = rate !== 1;
      const silencePause = silencePauseEnabled ? CUT_SILENCE_PAUSE_SECONDS : 0;

      setIsLoading(true);
      stopSources();

      try {
        const context = getContext();
        if (context.state === 'suspended') {
          await context.resume();
        }

        const loaded = await loadFullBuffer(sample);
        const { buffer: full, plans: availablePlans } = loaded;
        const split =
          availablePlans[fragmentIndex] ?? availablePlans[0] ?? null;

        if (!split || split.halfSeconds < 0.25) {
          setStatus(
            split
              ? `Cannot build equal halves around speech cut @ ${split.cutSeconds.toFixed(
                  3
                )}s (half=${split.halfSeconds.toFixed(3)}s).`
              : 'No speech fragment available.'
          );
          return;
        }

        const halfContent = split.halfSeconds;
        const firstHalfWallSeconds = halfContent / rate;
        const fullWallSeconds = (halfContent * 2) / rate + silencePause;
        const t0 = context.currentTime + START_DELAY_SECONDS;
        const joinWall = t0 + firstHalfWallSeconds;
        const secondStart = joinWall + silencePause;

        // Unbroken without a pause: one continuous buffer. With a pause (or glued):
        // two halves so we can insert wall-clock silence between them.
        const playAsTwoHalves = mode === 'glued' || silencePause > 0;

        if (!playAsTwoHalves) {
          const continuous = sliceAudioBuffer(
            context,
            full,
            split.regionStartSeconds,
            split.regionDurationSeconds
          );
          const source = context.createBufferSource({
            pitchCorrection
          });
          source.playbackRate.value = rate;
          source.buffer = continuous;
          source.connect(context.destination);
          sourcesRef.current = [source];

          source.onEnded = () => {
            setStatus(
              (prev) =>
                `${prev}\nfull ended @ ${context.currentTime.toFixed(3)}s`
            );
            source.disconnect();
            setIsRunning(false);
          };
          source.start(t0);
          setIsRunning(true);

          setStatus(
            [
              `sample=${sample} mode=unbroken (no cut, no silence pause)`,
              `pitchCorrection=${pitchCorrection} rate=${rate}`,
              `speech cut would be @ content ${halfContent.toFixed(3)}s of pruned clip`,
              `  (fragment #${split.fragmentIndex}, absolute ${split.cutSeconds.toFixed(3)}s, rms=${split.cutRms.toFixed(3)})`,
              `pruned=${continuous.duration.toFixed(2)}s  [${split.regionStartSeconds.toFixed(
                3
              )}, ${(
                split.regionStartSeconds + split.regionDurationSeconds
              ).toFixed(3)})s`,
              `cut → wall ~${joinWall.toFixed(3)}s`,
              `source.start(${t0.toFixed(3)})  wallDuration=${(halfContent * 2 / rate).toFixed(3)}s`,
            ].join('\n')
          );
          return;
        }

        const firstHalf = sliceAudioBuffer(
          context,
          full,
          split.regionStartSeconds,
          halfContent
        );
        const secondHalf = sliceAudioBuffer(
          context,
          full,
          split.cutSeconds,
          halfContent
        );

        const source1 = context.createBufferSource({
          pitchCorrection,
        });
        source1.playbackRate.value = rate;
        const source2 = context.createBufferSource({
          pitchCorrection,
        });
        source2.playbackRate.value = rate;

        source1.buffer = firstHalf;
        source2.buffer = secondHalf;

        source1.connect(context.destination);
        source2.connect(context.destination);

        sourcesRef.current = [source1, source2];

        source1.onEnded = () => {
          setStatus(
            (prev) =>
              `${prev}\nfirst ended @ ${context.currentTime.toFixed(3)}s (expected ~${joinWall.toFixed(3)}) ← cut/join`
          );
          source1.disconnect();
        };
        source2.onEnded = () => {
          setStatus(
            (prev) =>
              `${prev}\nsecond ended @ ${context.currentTime.toFixed(3)}s`
          );
          source2.disconnect();
          setIsRunning(false);
        };

        source1.start(t0);
        source2.start(t0 + firstHalf.duration);
        setIsRunning(true);

        setStatus(
          [
            `sample=${sample} mode=${mode}`,
            `pitchCorrection=${pitchCorrection} rate=${rate}`,
            `silencePause=${silencePause > 0 ? `${silencePause}s wall` : 'off'}`,
            `fragment #${split.fragmentIndex} cut @ absolute ${split.cutSeconds.toFixed(3)}s (rms=${split.cutRms.toFixed(3)})`,
            `pruned 2×${halfContent.toFixed(2)}s around cut`,
            `halfA=[${split.regionStartSeconds.toFixed(3)}, ${split.cutSeconds.toFixed(
              3
            )}) + halfB=[${split.cutSeconds.toFixed(3)}, ${(
              split.cutSeconds + halfContent
            ).toFixed(3)})`,
            `join/endA ~${joinWall.toFixed(3)}s, halfB.start ~${secondStart.toFixed(3)}s`,
            `source1.start(${t0.toFixed(3)})`,
            `source2.start(${secondStart.toFixed(3)})`,
            `wallDuration≈${fullWallSeconds.toFixed(3)}s`,
          ].join('\n')
        );
      } catch (error) {
        console.error(error);
        setStatus(`Error: ${String(error)}`);
        setIsRunning(false);
      } finally {
        setIsLoading(false);
      }
    },
    [
      fragmentIndex,
      getContext,
      loadFullBuffer,
      playbackRate,
      sample,
      silencePauseEnabled,
      stopSources,
    ]
  );

  useEffect(() => {
    return () => {
      stopSources();
      fullBufferRef.current = null;
      contextRef.current?.close();
      contextRef.current = null;
    };
  }, [stopSources]);

  const busy = isLoading || isRunning;
  const playLabel = (idle: string) =>
    isLoading ? 'Loading…' : isRunning ? 'Running…' : idle;

  const halfContent = plan?.halfSeconds ?? TARGET_HALF_SECONDS;
  const silencePause = silencePauseEnabled ? CUT_SILENCE_PAUSE_SECONDS : 0;
  const halfWallSeconds = halfContent / playbackRate;
  const fullWallSeconds = (halfContent * 2) / playbackRate + silencePause;
  const rateLabel = playbackRate.toFixed(1);
  const halfWallLabel = halfWallSeconds.toFixed(2);
  const fullWallLabel = fullWallSeconds.toFixed(2);
  const halfContentLabel = halfContent.toFixed(2);
  const cutLabel = plan ? plan.cutSeconds.toFixed(2) : '…';
  const rmsLabel = plan ? plan.cutRms.toFixed(3) : '…';

  return (
    <Container>
      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.body}
        keyboardShouldPersistTaps="handled"
      >
        <Text style={styles.title}>WSOLA schedule split</Text>
        <Text style={styles.caption}>
          Detects up to {MAX_SPEECH_FRAGMENTS} distinct high-energy speech peaks
          (≥ {MIN_PEAK_SEPARATION_SECONDS} s apart), then prunes equal halves ≤{' '}
          {TARGET_HALF_SECONDS}s around the selected cut. Optional{' '}
          {CUT_SILENCE_PAUSE_SECONDS} s wall silence marks the join. Rate 1 = no
          pitch correction; ≠ 1 = WSOLA.
        </Text>
        <Spacer.Vertical size={24} />
        <Text style={styles.sectionLabel}>Sample</Text>
        <Spacer.Vertical size={8} />
        {(['voice', 'track3', 'dcOnes'] as SampleAsset[]).map((key, index) => (
          <View key={key}>
            {index > 0 ? <Spacer.Vertical size={8} /> : null}
            <Button
              title={`${sample === key ? '●' : '○'} ${SAMPLE_LABELS[key]}`}
              onPress={() => setSample(key)}
              disabled={busy}
            />
          </View>
        ))}
        <Spacer.Vertical size={24} />
        <Text style={styles.sectionLabel}>Speech cut fragment</Text>
        <Spacer.Vertical size={8} />
        {plans.length === 0 ? (
          <Text style={styles.caption}>Scanning…</Text>
        ) : (
          plans.map((candidate, index) => (
            <View key={candidate.fragmentIndex}>
              {index > 0 ? <Spacer.Vertical size={8} /> : null}
              <Button
                title={`${fragmentIndex === index ? '●' : '○'} #${candidate.fragmentIndex} @ ${candidate.cutSeconds.toFixed(2)}s (rms ${candidate.cutRms.toFixed(3)})`}
                onPress={() => selectFragment(index)}
                disabled={busy}
              />
            </View>
          ))
        )}
        <Spacer.Vertical size={24} />
        <Text style={styles.sectionLabel}>Playback rate</Text>
        <Spacer.Vertical size={8} />
        <Slider
          label="Rate"
          value={playbackRate}
          onValueChange={setPlaybackRate}
          min={MIN_PLAYBACK_RATE}
          max={MAX_PLAYBACK_RATE}
          step={PLAYBACK_RATE_STEP}
          minLabelWidth={40}
        />
        <Spacer.Vertical size={16} />
        <Button
          title={`${silencePauseEnabled ? '●' : '○'} Cut silence pause (${CUT_SILENCE_PAUSE_SECONDS} s)`}
          onPress={() => setSilencePauseEnabled((prev) => !prev)}
          disabled={busy}
        />
        <Spacer.Vertical size={24} />
        <Text style={styles.sectionLabel}>
          Fragment #{plan?.fragmentIndex ?? '…'} cut @ {cutLabel}s (rms {rmsLabel})
          · 2×{halfContentLabel}s content → ~{fullWallLabel}s wall
          {silencePause > 0
            ? ` (join @ ${halfWallLabel}s + ${silencePause}s silence)`
            : ` (join @ ${halfWallLabel}s)`}
        </Text>
        <Spacer.Vertical size={8} />
        <Button
          title={playLabel(
            `Play glued (rate ${rateLabel}, ~${fullWallLabel}s wall)`
          )}
          onPress={() => runExperiment('glued')}
          disabled={busy || !plan}
        />
        <Spacer.Vertical size={24} />
        <Text style={styles.sectionLabel}>
          Unbroken same pruned region → ~{fullWallLabel}s wall
        </Text>
        <Spacer.Vertical size={8} />
        <Button
          title={playLabel(
            `Play unbroken (rate ${rateLabel}, ~${fullWallLabel}s wall)`
          )}
          onPress={() => runExperiment('unbroken')}
          disabled={busy || !plan}
        />
        <Spacer.Vertical size={12} />
        <Button title="Stop" onPress={stopSources} disabled={!isRunning} />
        <Spacer.Vertical size={24} />
        <Text style={styles.status}>{status}</Text>
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  scroll: {
    flex: 1,
  },
  body: {
    paddingHorizontal: 20,
    paddingTop: 24,
    paddingBottom: 40,
  },
  title: {
    color: colors.white,
    fontSize: 20,
    fontWeight: '600',
  },
  caption: {
    color: colors.white,
    opacity: 0.7,
    marginTop: 8,
    lineHeight: 20,
  },
  sectionLabel: {
    color: colors.white,
    fontSize: 14,
    fontWeight: '600',
    opacity: 0.85,
  },
  status: {
    color: colors.white,
    fontFamily: 'Courier',
    fontSize: 12,
    lineHeight: 18,
  },
});

export default WsolaScheduleSplit;
