import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { StyleSheet, Text } from 'react-native';
import { ScrollView } from 'react-native-gesture-handler';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors } from '../../styles';

/**
 * Mid-speech hard cut (example-voice-01, continuous region ~13.85–19.4s):
 *   2.75s + 2.75s — join sits inside continuous speech.
 * Schedule:
 *   b1.start(t0)
 *   b2.start(t0 + b1.duration / rate + L)
 * L defaults to 0; slider is for join experiments.
 */
// eslint-disable-next-line @typescript-eslint/no-var-requires
const SPEECH_B1 = require('./assets/speech-b1.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const SPEECH_B2 = require('./assets/speech-b2.wav') as number;

const START_DELAY_SECONDS = 0.25;
const MIN_PLAYBACK_RATE = 0.5;
const MAX_PLAYBACK_RATE = 2;
const PLAYBACK_RATE_STEP = 0.1;
const MIN_JOIN_COMPENSATION_MS = -100;
const MAX_JOIN_COMPENSATION_MS = 100;
const JOIN_COMPENSATION_STEP_MS = 1;
const DEFAULT_JOIN_COMPENSATION_MS = 0;

const SpeechScheduleSplit: FC = () => {
  const [playbackRate, setPlaybackRate] = useState(1.3);
  const [pitchCorrection, setPitchCorrection] = useState(true);
  const [joinCompensationMs, setJoinCompensationMs] = useState(
    DEFAULT_JOIN_COMPENSATION_MS
  );
  const [isLoading, setIsLoading] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [status, setStatus] = useState(
    'Mid-speech b1|b2 → start(t0 + duration/rate + L). L defaults to 0.'
  );
  const [eventLog, setEventLog] = useState<string[]>([]);

  const contextRef = useRef<AudioContext | null>(null);
  const sourcesRef = useRef<AudioBufferSourceNode[]>([]);
  const bufferPairRef = useRef<{ b1: AudioBuffer; b2: AudioBuffer } | null>(
    null
  );

  const appendLog = useCallback((line: string) => {
    setEventLog((prev) => [...prev, line]);
  }, []);

  // Drop stale decoded halves if bundled assets change (e.g. hot reload).
  useEffect(() => {
    bufferPairRef.current = null;
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

  useEffect(() => () => stopSources(), [stopSources]);

  const ensureBuffers = useCallback(async () => {
    if (!contextRef.current) {
      contextRef.current = new AudioContext();
    }
    const context = contextRef.current;
    if (context.state === 'suspended') {
      await context.resume();
    }

    if (!bufferPairRef.current) {
      const b1 = await context.decodeAudioData(SPEECH_B1);
      const b2 = await context.decodeAudioData(SPEECH_B2);
      bufferPairRef.current = { b1, b2 };
    }
    return { context, ...bufferPairRef.current };
  }, []);

  const playGlued = useCallback(async () => {
    setIsLoading(true);
    stopSources();
    setEventLog([]);

    try {
      const { context, b1, b2 } = await ensureBuffers();
      const rate = playbackRate;
      const L = joinCompensationMs / 1000;

      const source1 = context.createBufferSource({ pitchCorrection });
      source1.buffer = b1;
      source1.playbackRate.value = rate;

      const source2 = context.createBufferSource({ pitchCorrection });
      source2.buffer = b2;
      source2.playbackRate.value = rate;

      const t0 = context.currentTime + START_DELAY_SECONDS;
      const contentDur = b1.duration / rate;
      const joinAt = t0 + contentDur + L;

      source1.connect(context.destination);
      source2.connect(context.destination);
      sourcesRef.current = [source1, source2];

      source1.onEnded = () => {
        const now = context.currentTime;
        const deltaMs = (now - (t0 + contentDur)) * 1000;
        try {
          source1.disconnect();
        } catch {
          // already disconnected
        }
        appendLog(
          `b1 ended @ ${now.toFixed(3)}  expected ${ (t0 + contentDur).toFixed(3) }  Δ ${deltaMs.toFixed(1)} ms`
        );
      };

      source2.onEnded = () => {
        const now = context.currentTime;
        const expectedEnd = joinAt + b2.duration / rate;
        const deltaMs = (now - expectedEnd) * 1000;
        try {
          source2.disconnect();
        } catch {
          // already disconnected
        }
        setIsRunning(false);
        appendLog(
          `b2 ended @ ${now.toFixed(3)}  expected ${expectedEnd.toFixed(3)}  Δ ${deltaMs.toFixed(1)} ms`
        );
      };

      source1.start(t0);
      source2.start(joinAt);
      setIsRunning(true);

      const mode = pitchCorrection ? 'WSOLA' : 'raw';
      console.log('[SpeechScheduleSplit] schedule', {
        mode,
        rate,
        Lms: joinCompensationMs,
        t0,
        joinAt,
        b1Duration: b1.duration,
        b2Duration: b2.duration,
      });

      setStatus(
        [
          `glued · ${mode} · rate=${rate.toFixed(1)} · L=${joinCompensationMs} ms`,
          `b1=${b1.duration.toFixed(3)}s  b2=${b2.duration.toFixed(3)}s`,
          `contentDur=b1/rate=${contentDur.toFixed(4)}s`,
          `b1.start(${t0.toFixed(3)})`,
          `b2.start(${joinAt.toFixed(3)})  ← t0 + duration/rate + L`,
        ].join('\n')
      );
      appendLog(
        `scheduled · ${mode} · rate=${rate.toFixed(1)} · L=${joinCompensationMs}ms · join@${joinAt.toFixed(3)}`
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
    ensureBuffers,
    joinCompensationMs,
    pitchCorrection,
    playbackRate,
    stopSources,
  ]);

  const playUnbroken = useCallback(async () => {
    setIsLoading(true);
    stopSources();
    setEventLog([]);

    try {
      const { context, b1, b2 } = await ensureBuffers();
      const rate = playbackRate;

      const continuous = context.createBuffer(
        b1.numberOfChannels,
        b1.length + b2.length,
        b1.sampleRate
      );
      for (let ch = 0; ch < b1.numberOfChannels; ch += 1) {
        const out = continuous.getChannelData(ch);
        out.set(b1.getChannelData(ch), 0);
        out.set(b2.getChannelData(ch), b1.length);
      }

      const source = context.createBufferSource({ pitchCorrection });
      source.buffer = continuous;
      source.playbackRate.value = rate;
      source.connect(context.destination);
      sourcesRef.current = [source];

      const t0 = context.currentTime + START_DELAY_SECONDS;
      const joinWouldBe = t0 + b1.duration / rate;
      source.onEnded = () => {
        try {
          source.disconnect();
        } catch {
          // already disconnected
        }
        setIsRunning(false);
        appendLog(`unbroken ended @ ${context.currentTime.toFixed(3)}`);
      };
      source.start(t0);
      setIsRunning(true);

      const mode = pitchCorrection ? 'WSOLA' : 'raw';
      setStatus(
        [
          `unbroken · ${mode} · rate=${rate.toFixed(1)}`,
          `duration=${continuous.duration.toFixed(3)}s`,
          `join would be @ ${joinWouldBe.toFixed(3)} (no L)`,
        ].join('\n')
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
    ensureBuffers,
    pitchCorrection,
    playbackRate,
    stopSources,
  ]);

  const busy = isLoading || isRunning;

  return (
    <Container>
      <ScrollView contentContainerStyle={styles.body}>
        <Text style={styles.title}>Speech schedule split</Text>
        <Text style={styles.hint}>
          Mid-speech cut (2.75s + 2.75s). Compare glued b1|b2 vs unbroken, with /
          without WSOLA, tune rate and L (default 0).
        </Text>

        <Spacer.Vertical size={16} />
        <Text style={styles.section}>WSOLA</Text>
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

        <Spacer.Vertical size={16} />
        <Text style={styles.section}>Playback rate</Text>
        <Slider
          label={`rate ${playbackRate.toFixed(1)}`}
          value={playbackRate}
          min={MIN_PLAYBACK_RATE}
          max={MAX_PLAYBACK_RATE}
          step={PLAYBACK_RATE_STEP}
          onValueChange={setPlaybackRate}
        />

        <Spacer.Vertical size={16} />
        <Text style={styles.section}>
          Join latency L (ms) — b2.start(t0 + dur/rate + L)
        </Text>
        <Slider
          label={`L ${joinCompensationMs} ms`}
          value={joinCompensationMs}
          min={MIN_JOIN_COMPENSATION_MS}
          max={MAX_JOIN_COMPENSATION_MS}
          step={JOIN_COMPENSATION_STEP_MS}
          onValueChange={setJoinCompensationMs}
        />
        <Spacer.Vertical size={8} />
        <Button
          title="Reset L → 0"
          onPress={() => setJoinCompensationMs(DEFAULT_JOIN_COMPENSATION_MS)}
          disabled={busy || joinCompensationMs === 0}
        />

        <Spacer.Vertical size={16} />
        <Text style={styles.section}>Play</Text>
        <Button
          title={isLoading ? 'Loading…' : 'Play glued b1 → b2'}
          onPress={playGlued}
          disabled={isLoading}
        />
        <Spacer.Vertical size={8} />
        <Button
          title={isLoading ? 'Loading…' : 'Play unbroken (reference)'}
          onPress={playUnbroken}
          disabled={isLoading}
        />
        <Spacer.Vertical size={8} />
        <Button title="Stop" onPress={stopSources} disabled={!busy} />

        <Spacer.Vertical size={16} />
        <Text style={styles.status}>{status}</Text>
        {eventLog.length > 0 && (
          <>
            <Spacer.Vertical size={12} />
            <Text style={styles.section}>Events</Text>
            {eventLog.map((line, index) => (
              <Text key={`${index}-${line}`} style={styles.log}>
                {line}
              </Text>
            ))}
          </>
        )}
      </ScrollView>
    </Container>
  );
};

export default SpeechScheduleSplit;

const styles = StyleSheet.create({
  body: {
    padding: 16,
    paddingBottom: 48,
  },
  title: {
    fontSize: 20,
    fontWeight: '700',
    color: colors.white,
  },
  hint: {
    marginTop: 8,
    opacity: 0.75,
    color: colors.white,
  },
  section: {
    fontSize: 14,
    fontWeight: '600',
    color: colors.white,
    opacity: 0.9,
    marginBottom: 8,
  },
  status: {
    color: colors.white,
    fontFamily: 'Courier',
    fontSize: 12,
    lineHeight: 18,
  },
  log: {
    color: colors.white,
    fontFamily: 'Courier',
    fontSize: 11,
    lineHeight: 16,
    opacity: 0.85,
    marginBottom: 4,
  },
});
