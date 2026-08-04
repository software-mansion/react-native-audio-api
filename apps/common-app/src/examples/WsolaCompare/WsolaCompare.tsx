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

// eslint-disable-next-line @typescript-eslint/no-var-requires
const MUSIC_A = require('./assets/music-a-15s.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const MUSIC_B = require('./assets/music-b-5s.wav') as number;
// eslint-disable-next-line @typescript-eslint/no-var-requires
const VOICE = require('./assets/voice-5s.wav') as number;

type SampleKey = 'musicA' | 'musicB' | 'voice';

const SAMPLES: Record<
  SampleKey,
  { label: string; hint: string; asset: number }
> = {
  musicA: {
    label: 'Music A (15s)',
    hint: 'Full track 4 — bundled WAV',
    asset: MUSIC_A,
  },
  musicB: {
    label: 'Music B (5s)',
    hint: 'Continuous music excerpt — bundled WAV',
    asset: MUSIC_B,
  },
  voice: {
    label: 'Voice (~5.5s)',
    hint: 'Continuous speech excerpt — bundled WAV',
    asset: VOICE,
  },
};

const MIN_PLAYBACK_RATE = 0.5;
const MAX_PLAYBACK_RATE = 2;
const PLAYBACK_RATE_STEP = 0.1;
const START_DELAY_SECONDS = 0.15;

const WsolaCompare: FC = () => {
  const [sample, setSample] = useState<SampleKey>('musicA');
  const [playbackRate, setPlaybackRate] = useState(1.3);
  const [isLoading, setIsLoading] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [activeMode, setActiveMode] = useState<'wsola' | 'raw' | null>(null);
  const [status, setStatus] = useState(
    'Pick a track, set rate, then play with / without WSOLA'
  );

  const contextRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const bufferCacheRef = useRef<Partial<Record<SampleKey, AudioBuffer>>>({});

  const stopSource = useCallback(() => {
    const source = sourceRef.current;
    if (source) {
      try {
        source.stop(0);
      } catch {
        // already stopped
      }
      try {
        source.disconnect();
      } catch {
        // already disconnected
      }
    }
    sourceRef.current = null;
    setIsRunning(false);
    setActiveMode(null);
  }, []);

  useEffect(() => () => stopSource(), [stopSource]);

  // Drop stale decoded buffers when the bundled assets change (e.g. hot reload).
  useEffect(() => {
    bufferCacheRef.current = {};
  }, []);

  const ensureBuffer = useCallback(async (key: SampleKey) => {
    if (!contextRef.current) {
      contextRef.current = new AudioContext();
    }
    const context = contextRef.current;

    const cached = bufferCacheRef.current[key];
    if (cached) {
      return { context, buffer: cached };
    }

    const entry = SAMPLES[key];
    const buffer = await context.decodeAudioData(entry.asset);
    bufferCacheRef.current[key] = buffer;
    return { context, buffer };
  }, []);

  const play = useCallback(
    async (withWsola: boolean) => {
      setIsLoading(true);
      stopSource();

      try {
        const { context, buffer } = await ensureBuffer(sample);
        const rate = playbackRate;
        const source = context.createBufferSource({
          pitchCorrection: withWsola,
        });
        source.buffer = buffer;
        source.playbackRate.value = rate;
        source.connect(context.destination);

        const t0 = context.currentTime + START_DELAY_SECONDS;
        const wallDur = buffer.duration / rate;
        const expectedEnd = t0 + wallDur;
        source.onEnded = () => {
          const actualEnd = context.currentTime;
          const deltaMs = (actualEnd - expectedEnd) * 1000;
          const endLines = [
            `expectedEnd=${expectedEnd.toFixed(3)}s`,
            `actualEnd=${actualEnd.toFixed(3)}s`,
            `delta=${deltaMs >= 0 ? '+' : ''}${deltaMs.toFixed(1)} ms`,
          ];
          console.log('[WsolaCompare] end', {
            mode: withWsola ? 'wsola' : 'raw',
            rate,
            start: t0,
            expectedEnd,
            actualEnd,
            deltaMs,
          });
          setIsRunning(false);
          setActiveMode(null);
          setStatus(
            (prev) => `${prev}\n---\n${endLines.join('\n')}`
          );
          try {
            source.disconnect();
          } catch {
            // already disconnected
          }
          if (sourceRef.current === source) {
            sourceRef.current = null;
          }
        };

        source.start(t0);
        sourceRef.current = source;
        setIsRunning(true);
        setActiveMode(withWsola ? 'wsola' : 'raw');

        console.log('[WsolaCompare] start', {
          mode: withWsola ? 'wsola' : 'raw',
          rate,
          duration: buffer.duration,
          wallDur,
          start: t0,
          expectedEnd,
        });
        setStatus(
          [
            `${SAMPLES[sample].label}`,
            `mode=${withWsola ? 'WSOLA (pitchCorrection)' : 'raw (no WSOLA)'}`,
            `rate=${rate.toFixed(1)}  duration=${buffer.duration.toFixed(2)}s  wall≈${wallDur.toFixed(2)}s`,
            `start@${t0.toFixed(3)}`,
            `expectedEnd=${expectedEnd.toFixed(3)}s`,
          ].join('\n')
        );
      } catch (error) {
        console.error(error);
        setStatus(`Error: ${String(error)}`);
        setIsRunning(false);
        setActiveMode(null);
      } finally {
        setIsLoading(false);
      }
    },
    [ensureBuffer, playbackRate, sample, stopSource]
  );

  const busy = isLoading || isRunning;

  return (
    <Container>
      <ScrollView contentContainerStyle={styles.body}>
        <Text style={styles.title}>WSOLA compare</Text>
        <Text style={styles.hint}>
          Same track at the same rate — once through WSOLA, once without. Listen
          for pitch (chipmunk / dark) vs duration stretch.
        </Text>

        <Spacer.Vertical size={16} />
        <Text style={styles.section}>Track</Text>
        {(Object.keys(SAMPLES) as SampleKey[]).map((key) => {
          const selected = key === sample;
          return (
            <View key={key} style={styles.sampleRow}>
              <Button
                title={`${selected ? '● ' : '○ '}${SAMPLES[key].label}`}
                onPress={() => {
                  stopSource();
                  delete bufferCacheRef.current[key];
                  setSample(key);
                  setStatus(`Selected: ${SAMPLES[key].label}`);
                }}
                disabled={isLoading}
              />
              <Text style={styles.sampleHint}>{SAMPLES[key].hint}</Text>
            </View>
          );
        })}

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
        <Text style={styles.section}>Play</Text>
        <Button
          title={
            isLoading && activeMode === null
              ? 'Loading…'
              : activeMode === 'raw'
                ? '▶ Without WSOLA (playing)'
                : 'Play without WSOLA'
          }
          onPress={() => play(false)}
          disabled={isLoading}
        />
        <Spacer.Vertical size={8} />
        <Button
          title={
            isLoading && activeMode === null
              ? 'Loading…'
              : activeMode === 'wsola'
                ? '▶ With WSOLA (playing)'
                : 'Play with WSOLA'
          }
          onPress={() => play(true)}
          disabled={isLoading}
        />
        <Spacer.Vertical size={8} />
        <Button title="Stop" onPress={stopSource} disabled={!busy && !isRunning} />

        <Spacer.Vertical size={16} />
        <Text style={styles.status}>{status}</Text>
      </ScrollView>
    </Container>
  );
};

export default WsolaCompare;

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
  sampleRow: {
    marginBottom: 8,
  },
  sampleHint: {
    marginTop: 4,
    opacity: 0.6,
    color: colors.white,
    fontSize: 12,
  },
  status: {
    color: colors.white,
    fontFamily: 'Courier',
    fontSize: 12,
    lineHeight: 18,
  },
});
