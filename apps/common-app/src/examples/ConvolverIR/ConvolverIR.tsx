import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { StyleSheet, Text } from 'react-native';
import {
  AudioContext,
  AudioManager,
  ConvolverNode,
  GainNode,
  AudioBufferSourceNode,
  AudioBuffer,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors, layout } from '../../styles';

const LABEL_W = 88;
const IR = require('./ir.wav');
const TRACK = require('./track.mp3');

const ConvolverIR: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isReady, setIsReady] = useState(false);
  const [gain, setGain] = useState(0.28);

  const audioContextRef = useRef<AudioContext | null>(null);
  const impulseRef = useRef<AudioBuffer | null>(null);
  const bufferRef = useRef<AudioBuffer | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const convolverRef = useRef<ConvolverNode | null>(null);

  const teardownGraph = useCallback(() => {
    try {
      bufferSourceRef.current?.stop(0);
    } catch {
      /* not started */
    }
    bufferSourceRef.current?.disconnect();
    gainRef.current?.disconnect();
    convolverRef.current?.disconnect();
    bufferSourceRef.current = null;
    gainRef.current = null;
    convolverRef.current = null;
  }, []);

  useEffect(() => {
    const ctx = new AudioContext();
    audioContextRef.current = ctx;

    const downloadIR = async () => {
      const buffer = await ctx.decodeAudioData(IR);
      const track = await ctx.decodeAudioData(TRACK);
      impulseRef.current = buffer;
      bufferRef.current = track;
      setIsReady(true);
    };

    downloadIR().catch(() => {});

    return () => {
      teardownGraph();
      impulseRef.current = null;
      bufferRef.current = null;
      ctx.close().catch(() => {});
      audioContextRef.current = null;
      AudioManager.setAudioSessionActivity(false).catch(() => {});
    };
  }, []);

  const stopPlayback = useCallback(async () => {
    const ctx = audioContextRef.current;
    if (!ctx) {
      return;
    }
    bufferSourceRef.current?.stop(0);
    // await AudioManager.setAudioSessionActivity(false);
    setIsPlaying(false);
  }, []);

  const startPlayback = useCallback(
    async (useConvolver: boolean) => {
      const ctx = audioContextRef.current;
      const ir = impulseRef.current;
      const trackBuf = bufferRef.current;
      if (!ctx || !trackBuf || isPlaying) {
        return;
      }
      if (useConvolver && !ir) {
        return;
      }

      AudioManager.setAudioSessionOptions({
        iosCategory: 'playback',
        iosMode: 'default',
        iosOptions: [],
      });
      await AudioManager.setAudioSessionActivity(true);

      if (ctx.state === 'suspended') {
        await ctx.resume();
      }

      const source = ctx.createBufferSource();
      source.buffer = trackBuf;

      const g = ctx.createGain();
      g.gain.value = gain;

      if (useConvolver && ir) {
        const convolver = ctx.createConvolver();
        convolver.buffer = ir;
        source.connect(g).connect(convolver).connect(ctx.destination);
        convolverRef.current = convolver;
      } else {
        source.connect(g).connect(ctx.destination);
        convolverRef.current = null;
      }

      bufferSourceRef.current = source;
      gainRef.current = g;

      source.start(ctx.currentTime + 0.1, 10);
      setIsPlaying(true);
    },
    [gain, isPlaying],
  );

  const onPlayWithConvolver = useCallback(() => {
    startPlayback(true).catch(() => {});
  }, [startPlayback]);

  const onPlayDry = useCallback(() => {
    startPlayback(false).catch(() => {});
  }, [startPlayback]);

  const onStop = useCallback(() => {
    stopPlayback().catch(() => {});
  }, [stopPlayback]);

  const onGainChange = useCallback((v: number) => {
    setGain(v);
    if (gainRef.current) {
      gainRef.current.gain.value = v;
    }
  }, []);

  return (
    <Container centered>
      <Text style={styles.title}>Reverb</Text>
      <Spacer.Vertical size={10} />
      <Spacer.Vertical size={24} />
      <Button
        title="Play (convolver)"
        onPress={onPlayWithConvolver}
        width={200}
        disabled={!isReady || isPlaying}
      />
      <Spacer.Vertical size={12} />
      <Button
        title="Play (dry)"
        onPress={onPlayDry}
        width={200}
        disabled={!isReady || isPlaying}
      />
      <Spacer.Vertical size={12} />
      <Button title="Stop" onPress={onStop} width={200} disabled={!isPlaying} />
      <Spacer.Vertical size={28} />
      <Slider
        label="Gain"
        value={gain}
        onValueChange={onGainChange}
        min={0.02}
        max={0.6}
        step={0.01}
        minLabelWidth={LABEL_W}
      />
    </Container>
  );
};

export default ConvolverIR;

const styles = StyleSheet.create({
  title: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '600',
    textAlign: 'center',
  },
  copy: {
    color: colors.white,
    fontSize: 14,
    lineHeight: 20,
    opacity: 0.85,
    textAlign: 'center',
    paddingHorizontal: layout.spacing * 2,
  },
});
