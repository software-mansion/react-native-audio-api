import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { Image, StyleSheet, Text, View } from 'react-native';
import { ScrollView } from 'react-native-gesture-handler';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  OfflineAudioContext,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors } from '../../styles';

/**
 * Continuous sources cut with ffmpeg (first 5s + rest):
 *
 * Sustained chord pad (best join probe — steady level, no drums/silence):
 *   lavfi sine stack → source → -t 5 / -ss 5 → ffmpeg-pad-part{1,2}.wav
 *
 * Real music, smoother track (track4.mp3 — lowest level jumpiness):
 *   ffmpeg -i track4.mp3 -t 15 … → -t 5 / -ss 5 → ffmpeg-music-cont-part{1,2}.wav
 *
 * Tone control (440 Hz continuous):
 *   → ffmpeg-tone-part{1,2}.wav
 *
 * DC ones: synthetic fill(1) — offline dump of first frames counts WSOLA zeros.
 */
const START_DELAY_SECONDS = 0.4;
const MIN_PLAYBACK_RATE = 0.5;
const MAX_PLAYBACK_RATE = 3;
const PLAYBACK_RATE_STEP = 0.1;
/** Join pull-in (ms). Negative starts part2 earlier. Correct value is 0 (see C++ join tests). */
const MIN_JOIN_COMPENSATION_MS = -100;
const MAX_JOIN_COMPENSATION_MS = 100;
const JOIN_COMPENSATION_STEP_MS = 1;
const DEFAULT_JOIN_COMPENSATION_MS = 0;
/** Each half of the synthetic ones sample (seconds). */
const ONES_HALF_SECONDS = 5;
/** Match WsolaTimeStretcher::kFirstOutputFramesToDump. */
const LEADING_ZERO_PROBE_FRAMES = 255;
/** Extra offline length so cold-start latency still fits inside the capture. */
const LEADING_ZERO_CAPTURE_FRAMES = LEADING_ZERO_PROBE_FRAMES * 16;
const ZERO_THRESHOLD = 1e-6;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const PAD_P1 = require('./ffmpeg-pad-part1.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const PAD_P2 = require('./ffmpeg-pad-part2.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const MUSIC_CONT_P1 = require('./ffmpeg-music-cont-part1.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const MUSIC_CONT_P2 = require('./ffmpeg-music-cont-part2.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const TONE_P1 = require('./ffmpeg-tone-part1.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const TONE_P2 = require('./ffmpeg-tone-part2.wav') as number;

type SampleKey = 'pad' | 'musicCont' | 'tone' | 'ones';

type WavSample = {
  label: string;
  hint: string;
  part1?: number;
  part2?: number;
};

const SAMPLES: Record<SampleKey, WavSample> = {
  pad: {
    label: 'Continuous pad',
    hint: 'steady chord bed — best for hearing a join gap',
    part1: PAD_P1,
    part2: PAD_P2,
  },
  musicCont: {
    label: 'Music · track4 (smoother)',
    hint: 'real track, more continuous than track3',
    part1: MUSIC_CONT_P1,
    part2: MUSIC_CONT_P2,
  },
  tone: {
    label: 'Tone 440 Hz',
    hint: 'pure continuous tone — any join is obvious',
    part1: TONE_P1,
    part2: TONE_P2,
  },
  ones: {
    label: 'DC ones (1.0)',
    hint: 'synthetic fill(1) — use “Dump first frames” for leading zeros',
  },
};

function createDcOnesBuffer(
  context: AudioContext | OfflineAudioContext,
  durationSeconds: number
): AudioBuffer {
  const sampleRate = context.sampleRate;
  const length = Math.max(1, Math.floor(durationSeconds * sampleRate));
  const buffer = context.createBuffer(1, length, sampleRate);
  buffer.getChannelData(0).fill(1);
  return buffer;
}

function countLeadingZeros(samples: Float32Array, threshold = ZERO_THRESHOLD): number {
  let n = 0;
  while (n < samples.length && Math.abs(samples[n]!) < threshold) {
    n += 1;
  }
  return n;
}

function formatFrameDump(samples: Float32Array): string {
  const parts: string[] = [];
  for (let i = 0; i < samples.length; i += 1) {
    parts.push(`${i}:${samples[i]!.toFixed(4)}`);
  }
  return parts.join(' ');
}

/**
 * Experiment:
 *   ffmpeg-cut encoded WAV parts → decodeAudioData each →
 *   source1.start(t0); source2.start(t0 + buffer1.duration)
 */
const WavScheduleSplit: FC = () => {
  const [sample, setSample] = useState<SampleKey>('pad');
  const [playbackRate, setPlaybackRate] = useState(1);
  const [pitchCorrection, setPitchCorrection] = useState(true);
  const [joinCompensationMs, setJoinCompensationMs] = useState(
    DEFAULT_JOIN_COMPENSATION_MS
  );
  const [isLoading, setIsLoading] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [status, setStatus] = useState(
    'Ready — continuous WAV parts → decode → start(t0 + buffer1.duration / rate + L)'
  );
  const [eventLog, setEventLog] = useState<string[]>([]);
  const contextRef = useRef<AudioContext | null>(null);
  const sourcesRef = useRef<AudioBufferSourceNode[]>([]);

  const appendLog = useCallback((line: string) => {
    setEventLog((prev) => [...prev, line]);
  }, []);

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
      try {
        source.disconnect();
      } catch {
        // already disconnected
      }
    }
    sourcesRef.current = [];
    setIsRunning(false);
  }, []);

  useEffect(() => {
    return () => {
      stopSources();
      contextRef.current?.close();
      contextRef.current = null;
    };
  }, [stopSources]);

  const loadDecodedPair = useCallback(async () => {
    const context = getContext();
    if (context.state === 'suspended') {
      await context.resume();
    }

    if (sample === 'ones') {
      const buffer1 = createDcOnesBuffer(context, ONES_HALF_SECONDS);
      const buffer2 = createDcOnesBuffer(context, ONES_HALF_SECONDS);
      return { context, buffer1, buffer2 };
    }

    const { part1, part2 } = SAMPLES[sample];
    const uri1 = Image.resolveAssetSource(part1!).uri;
    const uri2 = Image.resolveAssetSource(part2!).uri;

    const arrayBuffer1 = await fetch(uri1).then((res) => res.arrayBuffer());
    const arrayBuffer2 = await fetch(uri2).then((res) => res.arrayBuffer());

    const buffer1 = await context.decodeAudioData(arrayBuffer1);
    const buffer2 = await context.decodeAudioData(arrayBuffer2);
    return { context, buffer1, buffer2 };
  }, [getContext, sample]);

  const dumpFirstOnesFrames = useCallback(async () => {
    setIsLoading(true);
    setEventLog([]);

    try {
      const live = getContext();
      if (live.state === 'suspended') {
        await live.resume();
      }

      const rate = playbackRate;
      const sampleRate = live.sampleRate;
      const offline = new OfflineAudioContext(
        1,
        LEADING_ZERO_CAPTURE_FRAMES,
        sampleRate
      );
      const ones = createDcOnesBuffer(
        offline,
        LEADING_ZERO_CAPTURE_FRAMES / sampleRate + 1
      );
      const source = offline.createBufferSource({ pitchCorrection });
      source.buffer = ones;
      source.playbackRate.value = rate;
      source.connect(offline.destination);
      source.start(0);

      const rendered = await offline.startRendering();
      const channel = rendered.getChannelData(0);
      const firstFrames = channel.subarray(0, LEADING_ZERO_PROBE_FRAMES);
      const leadingZeros = countLeadingZeros(firstFrames);
      const dump = formatFrameDump(firstFrames);

      console.log(
        `[WavScheduleSplit] first ${LEADING_ZERO_PROBE_FRAMES} frames · rate=${rate.toFixed(1)} · leadingZeros=${leadingZeros} · wsola=${pitchCorrection}`
      );
      console.log(`[WavScheduleSplit] frames: ${dump}`);

      setStatus(
        [
          `ones probe · rate=${rate.toFixed(1)} pitchCorrection=${pitchCorrection}`,
          `ctxSr=${sampleRate} capture=${LEADING_ZERO_CAPTURE_FRAMES}`,
          `leadingZeros in first ${LEADING_ZERO_PROBE_FRAMES}=${leadingZeros}`,
          `see console + event log for per-frame dump`,
        ].join('\n')
      );
      appendLog(
        `leadingZeros=${leadingZeros}/${LEADING_ZERO_PROBE_FRAMES} · rate=${rate.toFixed(1)} · wsola=${pitchCorrection}`
      );
      appendLog(dump);
    } catch (error) {
      console.error(error);
      setStatus(`Error: ${String(error)}`);
    } finally {
      setIsLoading(false);
    }
  }, [appendLog, getContext, pitchCorrection, playbackRate]);

  const playGlued = useCallback(async () => {
    setIsLoading(true);
    stopSources();
    setEventLog([]);

    try {
      const { context, buffer1, buffer2 } = await loadDecodedPair();
      const rate = playbackRate;

      const sourceNode1 = context.createBufferSource({ pitchCorrection });
      sourceNode1.buffer = buffer1;
      sourceNode1.playbackRate.value = rate;
      const sourceNode2 = context.createBufferSource({ pitchCorrection });
      sourceNode2.buffer = buffer2;
      sourceNode2.playbackRate.value = rate;

      const t0 = context.currentTime + START_DELAY_SECONDS;
      // Wall-clock length of part1 at this rate (content duration / rate).
      const contentDur = buffer1.duration / rate;
      // L only useful on the WSOLA path; without it the join is a plain hard splice.
      const joinCompensation = pitchCorrection
        ? joinCompensationMs / 1000
        : 0;
      const joinAt = t0 + contentDur + joinCompensation;

      sourceNode1.connect(context.destination);
      sourceNode2.connect(context.destination);
      sourcesRef.current = [sourceNode1, sourceNode2];

      sourceNode1.onEnded = () => {
        const now = context.currentTime;
        const expectedEnd = t0 + contentDur;
        const deltaMs = (now - expectedEnd) * 1000;
        try {
          sourceNode1.disconnect();
        } catch {
          // already disconnected
        }
        appendLog(
          `FIRST finished @ ${now.toFixed(3)}s  (content end ${expectedEnd.toFixed(3)}s, Δ ${deltaMs.toFixed(1)} ms; joinAt ${joinAt.toFixed(3)}s)`
        );
      };
      sourceNode2.onEnded = () => {
        const now = context.currentTime;
        const contentDur2 = buffer2.duration / rate;
        const expectedEnd2 = joinAt + contentDur2;
        const deltaMs2 = (now - expectedEnd2) * 1000;
        try {
          sourceNode2.disconnect();
        } catch {
          // already disconnected
        }
        setIsRunning(false);
        appendLog(
          `SECOND finished @ ${now.toFixed(3)}s  (content end ${expectedEnd2.toFixed(3)}s, Δ ${deltaMs2.toFixed(1)} ms; started ${joinAt.toFixed(3)}s)`
        );
      };

      sourceNode1.start(t0);
      sourceNode2.start(joinAt);
      setIsRunning(true);

      setStatus(
        [
          `glued · ${SAMPLES[sample].label} · rate=${rate.toFixed(1)} pitchCorrection=${pitchCorrection}`,
          SAMPLES[sample].hint,
          `buffer1.duration=${buffer1.duration.toFixed(4)}s len=${buffer1.length}`,
          `buffer2.duration=${buffer2.duration.toFixed(4)}s len=${buffer2.length}`,
          `contentDur=${contentDur.toFixed(4)}s (duration/rate)`,
          `joinCompensation=${(joinCompensation * 1000).toFixed(1)} ms`,
          `bufSr=${buffer1.sampleRate} ctxSr=${context.sampleRate}`,
          `source1.start(${t0.toFixed(3)})`,
          `source2.start(${joinAt.toFixed(3)})  ← joinAt (expected)`,
          `source2 expected end ${(joinAt + buffer2.duration / rate).toFixed(3)}s`,
          `Waiting for onEnded…`,
        ].join('\n')
      );
      appendLog(
        `scheduled · rate=${rate.toFixed(1)} · wsola=${pitchCorrection} · L=${(joinCompensation * 1000).toFixed(1)}ms · first→${t0.toFixed(3)}  second→${joinAt.toFixed(3)} · secondEnds~${(joinAt + buffer2.duration / rate).toFixed(3)}`
      );
    } catch (error) {
      console.error(error);
      setStatus(`Error: ${String(error)}`);
      setIsRunning(false);
    } finally {
      setIsLoading(false);
    }
  }, [
    appendLog,
    joinCompensationMs,
    loadDecodedPair,
    pitchCorrection,
    playbackRate,
    sample,
    stopSources,
  ]);

  const playUnbroken = useCallback(async () => {
    setIsLoading(true);
    stopSources();
    setEventLog([]);

    try {
      const { context, buffer1, buffer2 } = await loadDecodedPair();
      const rate = playbackRate;

      const length = buffer1.length + buffer2.length;
      const continuous = context.createBuffer(
        buffer1.numberOfChannels,
        length,
        buffer1.sampleRate
      );
      for (let ch = 0; ch < buffer1.numberOfChannels; ch += 1) {
        const out = continuous.getChannelData(ch);
        out.set(buffer1.getChannelData(ch), 0);
        out.set(buffer2.getChannelData(ch), buffer1.length);
      }

      const source = context.createBufferSource({ pitchCorrection });
      source.buffer = continuous;
      source.playbackRate.value = rate;
      source.connect(context.destination);
      sourcesRef.current = [source];

      const t0 = context.currentTime + START_DELAY_SECONDS;
      const joinWouldBe = t0 + buffer1.duration / rate;
      source.onEnded = () => {
        const now = context.currentTime;
        source.disconnect();
        setIsRunning(false);
        appendLog(`FULL finished @ ${now.toFixed(3)}s`);
      };
      source.start(t0);
      setIsRunning(true);

      setStatus(
        [
          `unbroken · ${SAMPLES[sample].label} · rate=${rate.toFixed(1)} pitchCorrection=${pitchCorrection}`,
          `duration=${continuous.duration.toFixed(4)}s`,
          `join would be @ ${joinWouldBe.toFixed(3)}s`,
          `source.start(${t0.toFixed(3)})`,
        ].join('\n')
      );
      appendLog(
        `scheduled unbroken · rate=${rate.toFixed(1)} · wsola=${pitchCorrection} · start ${t0.toFixed(3)}`
      );
    } catch (error) {
      console.error(error);
      setStatus(`Error: ${String(error)}`);
      setIsRunning(false);
    } finally {
      setIsLoading(false);
    }
  }, [appendLog, loadDecodedPair, pitchCorrection, playbackRate, sample, stopSources]);

  const busy = isLoading || isRunning;
  const playLabel = (idle: string) =>
    isLoading ? 'Loading…' : isRunning ? 'Running…' : idle;
  const rateLabel = playbackRate.toFixed(1);
  const joinLabel = `${joinCompensationMs} ms`;
  const wsolaLabel = pitchCorrection ? 'WSOLA' : 'raw';

  return (
    <Container>
      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.body}
        keyboardShouldPersistTaps="handled"
      >
        <Text style={styles.title}>WAV schedule split</Text>
        <Text style={styles.caption}>
          Continuous sources cut with ffmpeg (5 s + rest) → decode each →
          source2.start(t0 + duration/rate + L). Compare with / without WSOLA
          (especially at rate 1) and tune L on the WSOLA path.
        </Text>

        <Spacer.Vertical size={20} />
        <Text style={styles.section}>Sample</Text>
        {(Object.keys(SAMPLES) as SampleKey[]).map((key) => {
          const selected = sample === key;
          return (
            <View key={key} style={styles.sampleRow}>
              <Button
                title={`${selected ? '● ' : '○ '}${SAMPLES[key].label}`}
                onPress={() => {
                  if (!busy) {
                    setSample(key);
                    setEventLog([]);
                    setStatus(
                      `Selected: ${SAMPLES[key].label}\n${SAMPLES[key].hint}`
                    );
                  }
                }}
                disabled={busy}
              />
              <Text style={styles.hint}>{SAMPLES[key].hint}</Text>
            </View>
          );
        })}

        <Spacer.Vertical size={24} />
        <Text style={styles.section}>WSOLA</Text>
        <Text style={styles.hint}>
          pitchCorrection on buffer sources. At rate 1, off = plain playback;
          on = WSOLA still runs (prime/drain path).
        </Text>
        <Spacer.Vertical size={8} />
        <Button
          title={pitchCorrection ? '● With WSOLA' : '○ With WSOLA'}
          onPress={() => setPitchCorrection(true)}
          disabled={busy}
        />
        <Spacer.Vertical size={8} />
        <Button
          title={!pitchCorrection ? '● Without WSOLA' : '○ Without WSOLA'}
          onPress={() => setPitchCorrection(false)}
          disabled={busy}
        />

        <Spacer.Vertical size={24} />
        <Text style={styles.section}>Playback rate</Text>
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
        <Text style={styles.section}>Join compensation L</Text>
        <Text style={styles.hint}>
          Applied only when WSOLA is on. Without WSOLA the join is a hard
          splice at duration/rate (L ignored).
        </Text>
        <Spacer.Vertical size={8} />
        <Slider
          label="L ms"
          value={joinCompensationMs}
          onValueChange={setJoinCompensationMs}
          min={MIN_JOIN_COMPENSATION_MS}
          max={MAX_JOIN_COMPENSATION_MS}
          step={JOIN_COMPENSATION_STEP_MS}
          minLabelWidth={40}
        />

        <Spacer.Vertical size={24} />
        <Button
          title={playLabel(
            `Play glued (${wsolaLabel}, rate ${rateLabel}, L ${joinLabel})`
          )}
          onPress={() => void playGlued()}
          disabled={busy}
        />
        <Spacer.Vertical size={12} />
        <Button
          title={playLabel(`Play unbroken (${wsolaLabel}, rate ${rateLabel})`)}
          onPress={() => void playUnbroken()}
          disabled={busy}
        />
        <Spacer.Vertical size={12} />
        <Button
          title={
            isLoading
              ? 'Loading…'
              : `Dump first ${LEADING_ZERO_PROBE_FRAMES} frames (ones, ${wsolaLabel}, rate ${rateLabel})`
          }
          onPress={() => void dumpFirstOnesFrames()}
          disabled={busy}
        />
        <Spacer.Vertical size={12} />
        <Button title="Stop" onPress={stopSources} disabled={!isRunning} />

        <Spacer.Vertical size={24} />
        <Text style={styles.section}>Event log</Text>
        <Text style={styles.log}>
          {eventLog.length === 0
            ? '(no events yet — play glued and wait for onEnded)'
            : eventLog.join('\n')}
        </Text>

        <Spacer.Vertical size={16} />
        <Text style={styles.section}>Status</Text>
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
  section: {
    color: colors.white,
    fontWeight: '600',
    marginBottom: 8,
  },
  sampleRow: {
    marginBottom: 12,
  },
  hint: {
    color: colors.white,
    opacity: 0.55,
    fontSize: 12,
    marginTop: 4,
    marginLeft: 4,
  },
  log: {
    color: '#9BE7A0',
    fontFamily: 'Courier',
    fontSize: 13,
    lineHeight: 20,
    marginBottom: 8,
  },
  status: {
    color: colors.white,
    fontFamily: 'Courier',
    fontSize: 12,
    lineHeight: 18,
  },
});

export default WavScheduleSplit;
