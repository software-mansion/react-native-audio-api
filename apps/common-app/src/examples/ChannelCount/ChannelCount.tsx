import React, { useEffect, useRef, useState, FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  BaseAudioContext,
  GainNode,
  OfflineAudioContext,
} from 'react-native-audio-api';
import type {
  ChannelCountMode,
  ChannelInterpretation,
} from 'react-native-audio-api';

import { Container, Slider, Spacer, Button, Select } from '../../components';
import { colors, layout } from '../../styles';

// The source is a single buffer with SOURCE_CHANNELS channels, each carrying a
// distinct tone at an ascending amplitude (channel c peaks at (c + 1) /
// SOURCE_CHANNELS). Distinct per-channel content is what makes channelCount /
// channelCountMode / channelInterpretation changes measurable: with `discrete`
// the mix node keeps the first N channels and drops/zero-pads the rest, so the
// peak bars form a ramp that channelCount truncates. A stereo-only source would
// only ever populate 2 channels no matter how high channelCount goes.
const SOURCE_CHANNELS = 8;
const BASE_FREQUENCY = 220;

const CHANNEL_COUNT_MODES: ChannelCountMode[] = [
  'max',
  'clamped-max',
  'explicit',
];
const CHANNEL_INTERPRETATIONS: ChannelInterpretation[] = [
  'speakers',
  'discrete',
];

const ANALYSIS_CHANNELS = 8;
const labelWidth = 120;

// Builds a multi-channel test buffer. Channel c is a sine at
// BASE_FREQUENCY * (c + 1) scaled to amplitude (c + 1) / SOURCE_CHANNELS.
function createTestBuffer(ctx: BaseAudioContext, frames: number): AudioBuffer {
  const buffer = ctx.createBuffer(SOURCE_CHANNELS, frames, ctx.sampleRate);
  for (let c = 0; c < SOURCE_CHANNELS; c += 1) {
    const data = buffer.getChannelData(c);
    const frequency = BASE_FREQUENCY * (c + 1);
    const amplitude = (c + 1) / SOURCE_CHANNELS;
    for (let i = 0; i < frames; i += 1) {
      data[i] =
        amplitude * Math.sin((2 * Math.PI * frequency * i) / ctx.sampleRate);
    }
  }
  return buffer;
}

const ChannelCount: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [channelCount, setChannelCount] = useState(2);
  const [channelCountMode, setChannelCountMode] =
    useState<ChannelCountMode>('explicit');
  const [channelInterpretation, setChannelInterpretation] =
    useState<ChannelInterpretation>('discrete');
  const [analyzing, setAnalyzing] = useState(false);
  const [peaks, setPeaks] = useState<number[] | null>(null);

  const audioContextRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const mixRef = useRef<GainNode | null>(null);

  const setup = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }
    const ctx = audioContextRef.current;

    const source = ctx.createBufferSource();
    source.buffer = createTestBuffer(ctx, Math.floor(ctx.sampleRate));
    source.loop = true;

    const mix = ctx.createGain();
    mix.channelCountMode = channelCountMode;
    mix.channelInterpretation = channelInterpretation;
    mix.channelCount = channelCount;

    source.connect(mix);
    mix.connect(ctx.destination);

    sourceRef.current = source;
    mixRef.current = mix;
  };

  const teardown = () => {
    sourceRef.current?.stop(0);
    sourceRef.current = null;
    mixRef.current = null;
  };

  const handlePlayPause = () => {
    if (isPlaying) {
      teardown();
    } else {
      setup();
      sourceRef.current?.start(0);
    }

    setIsPlaying((prev) => !prev);
  };

  // Live-apply attribute changes to the currently playing mix node. This is the
  // real exercise for the renegotiation path: the graph must re-negotiate the
  // channel layout mid-render without a glitch or crash.
  const handleChannelCountChange = (value: number) => {
    const next = Math.round(value);
    setChannelCount(next);
    if (mixRef.current) {
      mixRef.current.channelCount = next;
    }
  };

  const handleModeChange = (value: ChannelCountMode) => {
    setChannelCountMode(value);
    if (mixRef.current) {
      mixRef.current.channelCountMode = value;
    }
  };

  const handleInterpretationChange = (value: ChannelInterpretation) => {
    setChannelInterpretation(value);
    if (mixRef.current) {
      mixRef.current.channelInterpretation = value;
    }
  };

  // Deterministic verification: render the same graph offline with the current
  // settings and report the peak amplitude of every output channel. The
  // destination is forced to discrete/explicit so per-channel content is
  // preserved for measurement instead of being mixed down.
  const handleAnalyze = async () => {
    if (analyzing) {
      return;
    }
    setAnalyzing(true);

    try {
      const sampleRate = 44100;
      const frames = 4096;
      const ctx = new OfflineAudioContext(
        ANALYSIS_CHANNELS,
        frames,
        sampleRate
      );
      ctx.destination.channelCount = ANALYSIS_CHANNELS;
      ctx.destination.channelCountMode = 'explicit';
      ctx.destination.channelInterpretation = 'discrete';

      const source = ctx.createBufferSource();
      source.buffer = createTestBuffer(ctx, frames);

      const mix = ctx.createGain();
      mix.channelCountMode = channelCountMode;
      mix.channelInterpretation = channelInterpretation;
      mix.channelCount = channelCount;

      source.connect(mix);
      mix.connect(ctx.destination);

      source.start(0);

      const rendered = await ctx.startRendering();

      const nextPeaks: number[] = [];
      for (let c = 0; c < rendered.numberOfChannels; c += 1) {
        const data = rendered.getChannelData(c);
        let peak = 0;
        for (let i = 0; i < data.length; i += 1) {
          const abs = Math.abs(data[i]);
          if (abs > peak) {
            peak = abs;
          }
        }
        nextPeaks.push(peak);
      }
      setPeaks(nextPeaks);
    } catch (error) {
      console.error('Channel analysis failed:', error);
    } finally {
      setAnalyzing(false);
    }
  };

  useEffect(() => {
    return () => {
      audioContextRef.current?.close();
      audioContextRef.current = null;
    };
  }, []);

  return (
    <Container centered>
      <Button onPress={handlePlayPause} title={isPlaying ? 'Stop' : 'Play'} />
      <Spacer.Vertical size={32} />

      <View style={styles.fullWidth}>
        <Slider
          label="Channels"
          value={channelCount}
          onValueChange={handleChannelCountChange}
          min={1}
          max={8}
          step={1}
          minLabelWidth={labelWidth}
        />
      </View>
      <Spacer.Vertical size={20} />

      <View style={styles.row}>
        <Text style={[styles.label, { minWidth: labelWidth }]}>Count mode</Text>
        <Spacer.Horizontal size={12} />
        <View style={styles.selectWrapper}>
          <Select
            value={channelCountMode}
            options={CHANNEL_COUNT_MODES}
            onChange={handleModeChange}
          />
        </View>
      </View>
      <Spacer.Vertical size={16} />

      <View style={styles.row}>
        <Text style={[styles.label, { minWidth: labelWidth }]}>
          Interpretation
        </Text>
        <Spacer.Horizontal size={12} />
        <View style={styles.selectWrapper}>
          <Select
            value={channelInterpretation}
            options={CHANNEL_INTERPRETATIONS}
            onChange={handleInterpretationChange}
          />
        </View>
      </View>
      <Spacer.Vertical size={32} />

      <Button
        onPress={handleAnalyze}
        title={analyzing ? 'Analyzing…' : 'Analyze output'}
        width={160}
        disabled={analyzing}
      />
      <Spacer.Vertical size={20} />

      {peaks && (
        <View style={styles.peaksContainer}>
          <Text style={styles.peaksTitle}>Per-channel peak amplitude</Text>
          <Spacer.Vertical size={12} />
          {peaks.map((peak, index) => (
            <View key={index} style={styles.peakRow}>
              <Text style={styles.peakLabel}>ch {index}</Text>
              <View style={styles.peakBarTrack}>
                <View
                  style={[
                    styles.peakBarFill,
                    { width: `${Math.min(1, peak) * 100}%` },
                  ]}
                />
              </View>
              <Text style={styles.peakValue}>{peak.toFixed(3)}</Text>
            </View>
          ))}
        </View>
      )}

      <Spacer.Vertical size={20} />
      <Text style={styles.hint}>
        Source has {SOURCE_CHANNELS} channels with ascending amplitude. With
        explicit/discrete, channelCount keeps the first N channels (the peak
        ramp truncates) and zero-pads the rest. Compare with speakers
        interpretation to see the up/down-mix summing rules.
      </Text>
    </Container>
  );
};

const styles = StyleSheet.create({
  fullWidth: {
    width: '100%',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    width: '100%',
  },
  label: {
    fontSize: 16,
    color: colors.white,
  },
  selectWrapper: {
    flex: 1,
  },
  peaksContainer: {
    width: '100%',
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: layout.radius,
    padding: layout.spacing,
  },
  peaksTitle: {
    fontSize: 16,
    fontWeight: '600',
    color: colors.white,
  },
  peakRow: {
    flexDirection: 'row',
    alignItems: 'center',
    marginVertical: 3,
  },
  peakLabel: {
    width: 44,
    color: colors.gray,
    fontSize: 13,
  },
  peakBarTrack: {
    flex: 1,
    height: 12,
    borderRadius: 6,
    backgroundColor: colors.backgroundLight,
    overflow: 'hidden',
  },
  peakBarFill: {
    height: 12,
    borderRadius: 6,
    backgroundColor: colors.main,
  },
  peakValue: {
    width: 56,
    textAlign: 'right',
    color: colors.white,
    fontSize: 13,
  },
  hint: {
    color: colors.gray,
    fontSize: 13,
    textAlign: 'center',
  },
});

export default ChannelCount;
