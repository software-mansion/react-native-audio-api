import React, { useRef, useState, FC } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  BaseAudioContext,
  GainNode,
  OfflineAudioContext,
} from 'react-native-audio-api';
import type {
  ChannelCountMode,
  ChannelInterpretation,
} from 'react-native-audio-api';

import {
  Button,
  Card,
  Container,
  ProgressBar,
  Select,
  Slider,
} from '../../components';
import { colors, typography } from '../../styles';
import { useAudioContext } from '../../utils/audio';

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

  const audioContext = useAudioContext();
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const mixRef = useRef<GainNode | null>(null);

  const setup = () => {
    const source = audioContext.createBufferSource();
    source.buffer = createTestBuffer(
      audioContext,
      Math.floor(audioContext.sampleRate)
    );
    source.loop = true;

    const mix = audioContext.createGain();
    mix.channelCountMode = channelCountMode;
    mix.channelInterpretation = channelInterpretation;
    mix.channelCount = channelCount;

    source.connect(mix);
    mix.connect(audioContext.destination);

    sourceRef.current = source;
    mixRef.current = mix;
  };

  const teardown = () => {
    sourceRef.current?.stop(0);
    sourceRef.current?.disconnect();
    mixRef.current?.disconnect();
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

  return (
    <Container>
      <ScrollView
        contentContainerStyle={styles.content}
        showsVerticalScrollIndicator={false}
      >
        <Button onPress={handlePlayPause} title={isPlaying ? 'Stop' : 'Play'} />
        <Slider
          label="Channels"
          value={channelCount}
          onValueChange={handleChannelCountChange}
          min={1}
          max={8}
          step={1}
        />
        <View style={styles.row}>
          <Text style={styles.label}>Count mode</Text>
          <View style={styles.selectWrapper}>
            <Select
              value={channelCountMode}
              options={CHANNEL_COUNT_MODES}
              onChange={handleModeChange}
            />
          </View>
        </View>
        <View style={styles.row}>
          <Text style={styles.label}>Interpretation</Text>
          <View style={styles.selectWrapper}>
            <Select
              value={channelInterpretation}
              options={CHANNEL_INTERPRETATIONS}
              onChange={handleInterpretationChange}
            />
          </View>
        </View>
        <Button
          onPress={handleAnalyze}
          title={analyzing ? 'Analyzing…' : 'Analyze output'}
          minWidth={160}
          disabled={analyzing}
        />
        {peaks && (
          <Card>
            <Text style={typography.sectionTitle}>
              Per-channel peak amplitude
            </Text>
            {peaks.map((peak, index) => (
              <View key={index} style={styles.peakRow}>
                <Text style={styles.peakLabel}>ch {index}</Text>
                <ProgressBar
                  progress={peak}
                  color={colors.blue}
                  height={12}
                  style={styles.peakBar}
                />
                <Text style={styles.peakValue}>{peak.toFixed(3)}</Text>
              </View>
            ))}
          </Card>
        )}
        <Text style={styles.hint}>
          Source has {SOURCE_CHANNELS} channels with ascending amplitude. With
          explicit/discrete, channelCount keeps the first N channels (the peak
          ramp truncates) and zero-pads the rest. Compare with speakers
          interpretation to see the up/down-mix summing rules.
        </Text>
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  content: {
    gap: 20,
    paddingBottom: 24,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  label: {
    minWidth: labelWidth,
    fontSize: 16,
    color: colors.textPrimary,
  },
  selectWrapper: {
    flex: 1,
  },
  peakRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  peakLabel: {
    width: 44,
    color: colors.textSecondary,
    fontSize: 13,
  },
  peakBar: {
    flex: 1,
  },
  peakValue: {
    width: 56,
    textAlign: 'right',
    color: colors.textPrimary,
    fontSize: 13,
    fontVariant: ['tabular-nums'],
  },
  hint: {
    ...typography.hint,
    textAlign: 'center',
  },
});

export default ChannelCount;
