import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { ScrollView } from 'react-native-gesture-handler';
import {
  AudioBuffer,
  AudioBufferQueueSourceNode,
  AudioContext,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer, Switch } from '../../components';
import { colors } from '../../styles';
import {
  FFMPEG_CHUNK_SECONDS,
  FFMPEG_CHUNKS,
  SampleKey,
} from './ffmpegChunks';

// TEMPORARY — remove after queue WSOLA drain validation.
// Chunks are cut offline with ffmpeg (./cut-chunks.sh), then decoded + enqueued.

const MIN_PLAYBACK_RATE = 0.5;
const MAX_PLAYBACK_RATE = 2;
const PLAYBACK_RATE_STEP = 0.1;

const QueueSourceTest: FC = () => {
  const [sample, setSample] = useState<SampleKey>('musicCont');
  const [playbackRate, setPlaybackRate] = useState(1.3);
  const [pitchCorrection, setPitchCorrection] = useState(true);
  const [callEndOfStream, setCallEndOfStream] = useState(true);
  const [isLoading, setIsLoading] = useState(false);
  const [isRunning, setIsRunning] = useState(false);
  const [status, setStatus] = useState(
    'Idle — ffmpeg ~1s chunks → enqueue → optional EOS → play'
  );
  const [eventLog, setEventLog] = useState<string[]>([]);

  const contextRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<AudioBufferQueueSourceNode | null>(null);
  const chunkCacheRef = useRef<Partial<Record<SampleKey, AudioBuffer[]>>>({});

  const appendLog = useCallback((line: string) => {
    setEventLog((prev) => [...prev, line]);
  }, []);

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
  }, []);

  useEffect(() => () => stopSource(), [stopSource]);

  const ensureDecodedChunks = useCallback(async (key: SampleKey) => {
    if (!contextRef.current) {
      contextRef.current = new AudioContext();
    }
    const context = contextRef.current;

    const cached = chunkCacheRef.current[key];
    if (cached) {
      return { context, chunks: cached };
    }

    const assets = FFMPEG_CHUNKS[key].assets;
    const chunks = await Promise.all(
      assets.map((asset) => context.decodeAudioData(asset))
    );
    chunkCacheRef.current[key] = chunks;
    return { context, chunks };
  }, []);

  const playQueued = useCallback(async () => {
    setIsLoading(true);
    stopSource();
    setEventLog([]);

    try {
      const { context, chunks } = await ensureDecodedChunks(sample);
      const rate = playbackRate;
      const usePitch = pitchCorrection;

      const source = context.createBufferQueueSource({
        pitchCorrection: usePitch,
        playbackRate: rate,
      });
      source.playbackRate.value = rate;

      const ids = chunks.map((chunk) => source.enqueueBuffer(chunk));

      if (callEndOfStream) {
        if (typeof source.endOfStream !== 'function') {
          throw new Error(
            'endOfStream() missing on native HostObject — rebuild the app (JS hot reload is not enough)'
          );
        }
        source.endOfStream();
      }

      const contentSeconds = chunks.reduce((sum, c) => sum + c.duration, 0);
      const contentDur = contentSeconds / rate;
      const t0 = context.currentTime + 0.2;

      source.onBufferEnded = (event) => {
        appendLog(
          `bufferEnded id=${event.bufferId} isLast=${event.isLastBufferInQueue}`
        );
      };
      source.onEnded = () => {
        const now = context.currentTime;
        const deltaMs = (now - (t0 + contentDur)) * 1000;
        appendLog(
          `onEnded @ ${now.toFixed(3)}s  expected~${(t0 + contentDur).toFixed(3)}s  Δ ${deltaMs.toFixed(1)} ms`
        );
        setIsRunning(false);
        try {
          source.disconnect();
        } catch {
          // already disconnected
        }
        if (sourceRef.current === source) {
          sourceRef.current = null;
        }
      };

      source.connect(context.destination);
      source.start(t0);
      sourceRef.current = source;
      setIsRunning(true);

      setStatus(
        [
          `TEMP queue · ${FFMPEG_CHUNKS[sample].label} (ffmpeg segments)`,
          `chunks=${ids.length} × ~${FFMPEG_CHUNK_SECONDS}s`,
          `rate=${rate.toFixed(1)} pitchCorrection=${usePitch} endOfStream=${callEndOfStream}`,
          `content=${contentSeconds.toFixed(3)}s wall≈${contentDur.toFixed(3)}s`,
          `start@${t0.toFixed(3)} · ids=[${ids.slice(0, 4).join(', ')}${ids.length > 4 ? ', …' : ''}]`,
        ].join('\n')
      );
      appendLog(
        `started · ${FFMPEG_CHUNKS[sample].label} · ${ids.length} ffmpeg chunks · EOS=${callEndOfStream} · pitch=${usePitch}`
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
    callEndOfStream,
    ensureDecodedChunks,
    pitchCorrection,
    playbackRate,
    sample,
    stopSource,
  ]);

  return (
    <Container>
      <ScrollView contentContainerStyle={styles.body}>
        <Text style={styles.title}>Queue Source Test (TEMP)</Text>
        <Text style={styles.hint}>
          Prefers ffmpeg-cut ~1s WAV files (./cut-chunks.sh) → decode each →
          enqueueBuffer → optional endOfStream → play.
        </Text>

        <Spacer.Vertical size={12} />
        {(Object.keys(FFMPEG_CHUNKS) as SampleKey[]).map((key) => {
          const selected = key === sample;
          return (
            <View key={key} style={styles.sampleRow}>
              <Button
                title={`${selected ? '● ' : '○ '}${FFMPEG_CHUNKS[key].label}`}
                onPress={() => {
                  stopSource();
                  setSample(key);
                  setStatus(`Selected: ${FFMPEG_CHUNKS[key].label}`);
                }}
                disabled={isLoading || isRunning}
              />
              <Text style={styles.sampleHint}>{FFMPEG_CHUNKS[key].hint}</Text>
            </View>
          );
        })}

        <Spacer.Vertical size={12} />
        <Slider
          label={`playbackRate ${playbackRate.toFixed(1)}`}
          value={playbackRate}
          min={MIN_PLAYBACK_RATE}
          max={MAX_PLAYBACK_RATE}
          step={PLAYBACK_RATE_STEP}
          onValueChange={setPlaybackRate}
        />

        <View style={styles.row}>
          <Text style={styles.switchLabel}>pitchCorrection</Text>
          <Switch value={pitchCorrection} onValueChange={setPitchCorrection} />
        </View>
        <View style={styles.row}>
          <Text style={styles.switchLabel}>endOfStream()</Text>
          <Switch value={callEndOfStream} onValueChange={setCallEndOfStream} />
        </View>

        <Spacer.Vertical size={12} />
        <Button
          title={isLoading ? 'Loading…' : 'Play queued'}
          onPress={playQueued}
          disabled={isLoading || isRunning}
        />
        <Spacer.Vertical size={8} />
        <Button title="Stop" onPress={stopSource} disabled={!isRunning} />

        <Spacer.Vertical size={16} />
        <Text style={styles.status}>{status}</Text>
        {eventLog.length > 0 && (
          <>
            <Spacer.Vertical size={8} />
            <Text style={styles.log}>{eventLog.join('\n')}</Text>
          </>
        )}
      </ScrollView>
    </Container>
  );
};

export default QueueSourceTest;

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
  sampleRow: {
    marginBottom: 8,
  },
  sampleHint: {
    marginTop: 4,
    opacity: 0.6,
    color: colors.white,
    fontSize: 12,
  },
  row: {
    marginTop: 8,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  switchLabel: {
    color: colors.white,
    fontSize: 16,
  },
  status: {
    color: colors.white,
    fontFamily: 'Courier',
    fontSize: 12,
    lineHeight: 18,
  },
  log: {
    color: colors.white,
    opacity: 0.85,
    fontFamily: 'Courier',
    fontSize: 12,
    lineHeight: 18,
  },
});
