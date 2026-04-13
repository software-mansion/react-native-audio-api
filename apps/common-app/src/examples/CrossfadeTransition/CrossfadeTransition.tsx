import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import {
  ActivityIndicator,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  AudioManager,
  GainNode,
} from 'react-native-audio-api';

import { Button, Container, Spacer } from '../../components';
import { colors } from '../../styles';

const TRACK_1 = require('./track1.mp3');
const TRACK_2 = require('./track3.mp3');

const INTRO_BEFORE_FADE_SEC = 3;
const FADE_DURATION_SEC = 5;
const CURVE_POINTS = 128;

const buildCurves = (): {
  curve1: Float32Array;
  curve2: Float32Array;
} => {
  const curve1 = new Float32Array(CURVE_POINTS);
  const curve2 = new Float32Array(CURVE_POINTS);
  for (let i = 0; i < CURVE_POINTS; i++) {
    const progress = i / (CURVE_POINTS - 1);
    curve1[i] = Math.cos(progress * 0.5 * Math.PI);
    curve2[i] = Math.cos((1 - progress) * 0.5 * Math.PI);
  }
  return { curve1, curve2 };
};

const CrossfadeTransition: FC = () => {
  const [isLoading, setIsLoading] = useState(true);
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContext = useRef<AudioContext | null>(null);
  const buffer1 = useRef<AudioBuffer | null>(null);
  const buffer2 = useRef<AudioBuffer | null>(null);
  const sourceNode1 = useRef<AudioBufferSourceNode | null>(null);
  const sourceNode2 = useRef<AudioBufferSourceNode | null>(null);
  const gainNode1 = useRef<GainNode | null>(null);
  const gainNode2 = useRef<GainNode | null>(null);

  const teardownPlayback = useCallback(() => {
    try {
      sourceNode1.current?.stop(0);
    } catch {
      /* already stopped */
    }
    try {
      sourceNode2.current?.stop(0);
    } catch {
      /* already stopped */
    }
    gainNode1.current?.disconnect();
    gainNode2.current?.disconnect();
    sourceNode1.current = null;
    sourceNode2.current = null;
    gainNode1.current = null;
    gainNode2.current = null;
  }, []);

  useEffect(() => {
    const init = async () => {
      audioContext.current = new AudioContext();

      if (buffer1.current && buffer2.current) {
        setIsLoading(false);
        return;
      }

      buffer1.current = await audioContext.current.decodeAudioData(TRACK_1);
      buffer2.current = await audioContext.current.decodeAudioData(TRACK_2);
      setIsLoading(false);
    };

    init().catch(() => {});

    return () => {
      teardownPlayback();
      const ctx = audioContext.current;
      if (ctx) {
        ctx.suspend().catch(() => {});
      }
      AudioManager.setAudioSessionActivity(false).catch(() => {});
    };
  }, [teardownPlayback]);

  const stopPlayback = useCallback(async () => {
    if (!audioContext.current) {
      return;
    }
    teardownPlayback();
    await audioContext.current.suspend();
    await AudioManager.setAudioSessionActivity(false);
    setIsPlaying(false);
  }, [teardownPlayback]);

  const startTransition = useCallback(async () => {
    const ctx = audioContext.current;
    const b1 = buffer1.current;
    const b2 = buffer2.current;
    if (!ctx || !b1 || !b2) {
      return;
    }

    if (isPlaying) {
      await stopPlayback();
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

    const s1 = ctx.createBufferSource();
    const s2 = ctx.createBufferSource();
    s1.buffer = b1;
    s2.buffer = b2;
    const g1 = ctx.createGain();
    const g2 = ctx.createGain();
    s1.connect(g1).connect(ctx.destination);
    s2.connect(g2).connect(ctx.destination);

    const now = ctx.currentTime + 0.05;
    const fadeStart = now + INTRO_BEFORE_FADE_SEC;
    const { curve1, curve2 } = buildCurves();

    g1.gain.setValueAtTime(1, now);
    g2.gain.setValueAtTime(0, now);
    g1.gain.setValueCurveAtTime(curve1, fadeStart, FADE_DURATION_SEC);
    g2.gain.setValueCurveAtTime(curve2, fadeStart, FADE_DURATION_SEC);

    s1.start(now, 17.8);
    s2.start(now, 62);

    sourceNode1.current = s1;
    sourceNode2.current = s2;
    gainNode1.current = g1;
    gainNode2.current = g2;

    setIsPlaying(true);
  }, [isPlaying, stopPlayback]);

  const startImmediateSwitch = useCallback(async () => {
    const ctx = audioContext.current;
    const b1 = buffer1.current;
    const b2 = buffer2.current;
    if (!ctx || !b1 || !b2) {
      return;
    }

    if (isPlaying) {
      await stopPlayback();
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

    const s1 = ctx.createBufferSource();
    const s2 = ctx.createBufferSource();
    s1.buffer = b1;
    s2.buffer = b2;
    const g1 = ctx.createGain();
    const g2 = ctx.createGain();
    s1.connect(g1).connect(ctx.destination);
    s2.connect(g2).connect(ctx.destination);

    const now = ctx.currentTime + 0.05;
    const switchTime = now + INTRO_BEFORE_FADE_SEC;

    g1.gain.setValueAtTime(1, now);
    g2.gain.setValueAtTime(0, now);
    g1.gain.setValueAtTime(0, switchTime);
    g2.gain.setValueAtTime(1, switchTime);

    s1.start(now, 17.8);
    s2.start(now, 62);

    sourceNode1.current = s1;
    sourceNode2.current = s2;
    gainNode1.current = g1;
    gainNode2.current = g2;

    setIsPlaying(true);
  }, [isPlaying, stopPlayback]);

  const onPrimaryPress = useCallback(() => {
    if (isPlaying) {
      stopPlayback().catch(() => {});
    } else {
      startTransition().catch(() => {});
    }
  }, [isPlaying, startTransition, stopPlayback]);

  const onImmediatePress = useCallback(() => {
    startImmediateSwitch().catch(() => {});
  }, [startImmediateSwitch]);

  return (
    <Container centered>
      {isLoading ? (
        <ActivityIndicator color={colors.white} />
      ) : (
        <View style={styles.body}>
          <Text style={styles.title}>Automated crossfade</Text>
          <Spacer.Vertical size={12} />
          <Text style={styles.copy}>
            Two tracks start together; you hear the first alone, then the mix
            moves to the second—either with a long equal-power fade or with a
            single scheduled step in gain (no ramp).
          </Text>
          <Spacer.Vertical size={28} />
          <Button
            title={isPlaying ? 'Stop' : 'Smooth crossfade'}
            onPress={onPrimaryPress}
            width={220}
          />
          <Spacer.Vertical size={16} />
          <Text style={styles.meta}>
            {INTRO_BEFORE_FADE_SEC}s intro, then {FADE_DURATION_SEC}s crossfade
          </Text>
          <Spacer.Vertical size={24} />
          <Button
            title="Immediate switch"
            onPress={onImmediatePress}
            width={220}
          />
          <Spacer.Vertical size={16} />
          <Text style={styles.meta}>
            Same intro, then both gains jump at the same instant (hard cut)
          </Text>
        </View>
      )}
    </Container>
  );
};

export default CrossfadeTransition;

const styles = StyleSheet.create({
  body: {
    paddingHorizontal: 24,
    alignItems: 'center',
  },
  title: {
    color: colors.white,
    fontSize: 20,
    fontWeight: '600',
    textAlign: 'center',
  },
  copy: {
    color: colors.white,
    fontSize: 15,
    lineHeight: 22,
    opacity: 0.85,
    textAlign: 'center',
  },
  meta: {
    color: colors.white,
    fontSize: 13,
    opacity: 0.65,
    textAlign: 'center',
  },
});
