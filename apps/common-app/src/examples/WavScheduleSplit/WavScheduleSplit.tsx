import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { Image, StyleSheet, Text, View } from 'react-native';
import { ScrollView } from 'react-native-gesture-handler';
import {
  AudioBufferSourceNode,
  AudioContext,
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
 */
const START_DELAY_SECONDS = 0.4;
const MIN_PLAYBACK_RATE = 0.5;
const MAX_PLAYBACK_RATE = 3;
const PLAYBACK_RATE_STEP = 0.1;
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

type SampleKey = 'pad' | 'musicCont' | 'tone';

const SAMPLES: Record<
  SampleKey,
  { label: string; hint: string; part1: number; part2: number }
> = {
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
};

/**
 * Experiment:
 *   ffmpeg-cut encoded WAV parts → decodeAudioData each →
 *   source1.start(t0); source2.start(t0 + buffer1.duration)
 */
const WavScheduleSplit: FC = () => {
  const [sample, setSample] = useState<SampleKey>('pad');
  const [playbackRate, setPlaybackRate] = useState(1);
  const [isLoading, setIsLoading] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [status, setStatus] = useState(
    'Ready — continuous WAV parts → decode → start(t0 + buffer1.duration / rate)'
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

    const { part1, part2 } = SAMPLES[sample];
    const uri1 = Image.resolveAssetSource(part1).uri;
    const uri2 = Image.resolveAssetSource(part2).uri;

    const arrayBuffer1 = await fetch(uri1).then((res) => res.arrayBuffer());
    const arrayBuffer2 = await fetch(uri2).then((res) => res.arrayBuffer());

    const buffer1 = await context.decodeAudioData(arrayBuffer1);
    const buffer2 = await context.decodeAudioData(arrayBuffer2);
    return { context, buffer1, buffer2 };
  }, [getContext, sample]);

  const playGlued = useCallback(async () => {
    setIsLoading(true);
    stopSources();
    setEventLog([]);

    try {
      const { context, buffer1, buffer2 } = await loadDecodedPair();
      const rate = playbackRate;
      const pitchCorrection = rate !== 1;

      const sourceNode1 = context.createBufferSource({ pitchCorrection });
      sourceNode1.buffer = buffer1;
      sourceNode1.playbackRate.value = rate;
      const sourceNode2 = context.createBufferSource({ pitchCorrection });
      sourceNode2.buffer = buffer2;
      sourceNode2.playbackRate.value = rate;

      const t0 = context.currentTime + START_DELAY_SECONDS;
      // Wall-clock length of part1 at this rate (content duration / rate).
      const contentDur = buffer1.duration / rate;
      const joinAt = t0 + contentDur;
      sourceNode1.connect(context.destination);
      sourceNode2.connect(context.destination);
      sourcesRef.current = [sourceNode1, sourceNode2];

      sourceNode1.onEnded = () => {
        const now = context.currentTime;
        const deltaMs = (now - joinAt) * 1000;
        sourceNode1.disconnect();
        appendLog(
          `FIRST finished @ ${now.toFixed(3)}s  (expected join ${joinAt.toFixed(3)}s, Δ ${deltaMs.toFixed(1)} ms)`
        );
      };
      sourceNode2.onEnded = () => {
        const now = context.currentTime;
        sourceNode2.disconnect();
        setIsRunning(false);
        appendLog(`SECOND finished @ ${now.toFixed(3)}s`);
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
          `bufSr=${buffer1.sampleRate} ctxSr=${context.sampleRate}`,
          `source1.start(${t0.toFixed(3)})`,
          `source2.start(${joinAt.toFixed(3)})  ← joinAt (expected)`,
          `Waiting for onEnded…`,
        ].join('\n')
      );
      appendLog(
        `scheduled · rate=${rate.toFixed(1)} · first→${t0.toFixed(3)}  second→${joinAt.toFixed(3)} (joinAt expected)`
      );
    } catch (error) {
      console.error(error);
      setStatus(`Error: ${String(error)}`);
      setIsRunning(false);
    } finally {
      setIsLoading(false);
    }
  }, [appendLog, loadDecodedPair, playbackRate, sample, stopSources]);

  const playUnbroken = useCallback(async () => {
    setIsLoading(true);
    stopSources();
    setEventLog([]);

    try {
      const { context, buffer1, buffer2 } = await loadDecodedPair();
      const rate = playbackRate;
      const pitchCorrection = rate !== 1;

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
      appendLog(`scheduled unbroken · rate=${rate.toFixed(1)} · start ${t0.toFixed(3)}`);
    } catch (error) {
      console.error(error);
      setStatus(`Error: ${String(error)}`);
      setIsRunning(false);
    } finally {
      setIsLoading(false);
    }
  }, [appendLog, loadDecodedPair, playbackRate, sample, stopSources]);

  const busy = isLoading || isRunning;
  const playLabel = (idle: string) =>
    isLoading ? 'Loading…' : isRunning ? 'Running…' : idle;
  const rateLabel = playbackRate.toFixed(1);

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
          source2.start(t0 + buffer1.duration). Prefer the pad to hear join
          gaps clearly.
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

        <Spacer.Vertical size={24} />
        <Button
          title={playLabel(`Play glued (rate ${rateLabel})`)}
          onPress={() => void playGlued()}
          disabled={busy}
        />
        <Spacer.Vertical size={12} />
        <Button
          title={playLabel(`Play unbroken (rate ${rateLabel})`)}
          onPress={() => void playUnbroken()}
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
