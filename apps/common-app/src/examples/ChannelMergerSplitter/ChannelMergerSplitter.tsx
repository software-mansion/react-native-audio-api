import React, { useCallback, useEffect, useRef, useState, FC } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  AudioManager,
  AudioNode,
  GainNode,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors } from '../../styles';

const SURROUND_5_1 =
  'https://www2.iis.fraunhofer.de/AAC/ChID-BLITS-EBU-Narration.mp4';

const CHANNEL_COUNT = 6;
const CHANNEL_LABELS = ['L', 'R', 'C', 'LFE', 'Ls', 'Rs'] as const;

const INITIAL_GAINS = [0.3, 0.6, 0.1, 1.0, 0.7, 0.7];

const labelWidth = 56;

/**
 * Graph:
 *   AudioBufferSourceNode (5.1) → ChannelSplitter(6)
 *     → per-channel GainNodes
 *     → ChannelMerger(2) stereo fold-down → destination
 *
 * Fold-down: L/C/LFE/Ls → left, R/C/LFE/Rs → right
 */
const ChannelMergerSplitter: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isReady, setIsReady] = useState(false);
  const [channelCount, setChannelCount] = useState(0);
  const [gains, setGains] = useState<number[]>([...INITIAL_GAINS]);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferRef = useRef<AudioBuffer | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const channelGainsRef = useRef<GainNode[]>([]);
  const outputNodeRef = useRef<AudioNode | null>(null);

  const teardown = useCallback(() => {
    try {
      sourceRef.current?.stop(0);
    } catch {
      // already stopped
    }

    outputNodeRef.current?.disconnect();
    sourceRef.current?.disconnect();

    sourceRef.current = null;
    channelGainsRef.current = [];
    outputNodeRef.current = null;
  }, []);

  const setup = useCallback(async () => {
    const ctx = audioContextRef.current;
    const buffer = bufferRef.current;
    if (!ctx || !buffer) {
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

    const numberOfChannels = Math.min(buffer.numberOfChannels, CHANNEL_COUNT);
    const splitter = ctx.createChannelSplitter(numberOfChannels);
    const stereoMerger = ctx.createChannelMerger(2);

    const source = ctx.createBufferSource();
    source.buffer = buffer;
    source.connect(splitter);

    const channelGains: GainNode[] = [];
    for (let i = 0; i < numberOfChannels; i++) {
      const gain = ctx.createGain();
      gain.gain.value = gains[i] ?? 0.7;
      splitter.connect(gain, i, 0);
      channelGains.push(gain);
    }

    // 5.1 → stereo fold-down
    const connectLeft = (index: number) => {
      channelGains[index]?.connect(stereoMerger, 0, 0);
    };
    const connectRight = (index: number) => {
      channelGains[index]?.connect(stereoMerger, 0, 1);
    };

    connectLeft(0); // L
    connectRight(1); // R
    connectLeft(2); // C
    connectRight(2);
    connectLeft(3); // LFE
    connectRight(3);
    connectLeft(4); // Ls
    connectRight(5); // Rs

    stereoMerger.connect(ctx.destination);

    sourceRef.current = source;
    channelGainsRef.current = channelGains;
    outputNodeRef.current = stereoMerger;

    source.onEnded = () => {
      setIsPlaying(false);
      teardown();
    };

    source.start(0);
  }, [gains, teardown]);

  const handlePlayPause = () => {
    if (isPlaying) {
      teardown();
      setIsPlaying(false);
      return;
    }

    setup()
      .then(() => setIsPlaying(true))
      .catch(() => {
        teardown();
        setIsPlaying(false);
      });
  };

  const handleGainChange = (channelIndex: number, value: number) => {
    setGains((prev) => {
      const next = [...prev];
      next[channelIndex] = value;
      return next;
    });

    const gainNode = channelGainsRef.current[channelIndex];
    if (gainNode) {
      gainNode.gain.value = value;
    }
  };

  useEffect(() => {
    const ctx = new AudioContext();
    audioContextRef.current = ctx;

    ctx
      .decodeAudioData(SURROUND_5_1)
      .then((buffer) => {
        bufferRef.current = buffer;
        setChannelCount(buffer.numberOfChannels);
        setIsReady(true);
      })
      .catch(() => {
        setIsReady(false);
      });

    return () => {
      teardown();
      bufferRef.current = null;
      ctx.close().catch(() => {});
      audioContextRef.current = null;
      AudioManager.setAudioSessionActivity(false).catch(() => {});
    };
  }, [teardown]);

  return (
    <Container>
      <ScrollView
        contentContainerStyle={styles.content}
        showsVerticalScrollIndicator={false}>
        <Text style={styles.description}>
          Plays a 5.1 surround test (L R C LFE Ls Rs) through
          AudioBufferSourceNode → ChannelSplitter → per-channel gains →
          ChannelMerger stereo fold-down. Solo a channel with its gain slider.
        </Text>

        <Spacer.Vertical size={8} />
        <Text style={styles.meta}>
          {isReady
            ? `Loaded ${channelCount}-channel buffer`
            : 'Loading 5.1 sample…'}
        </Text>

        <Spacer.Vertical size={16} />
        <Button
          onPress={handlePlayPause}
          title={isPlaying ? 'Stop' : 'Play'}
          disabled={!isReady}
        />
        <Spacer.Vertical size={24} />

        <Text style={styles.sectionTitle}>Channel gains</Text>
        <Spacer.Vertical size={8} />
        {CHANNEL_LABELS.map((label, index) => (
          <View key={label}>
            <Slider
              label={label}
              value={gains[index]}
              onValueChange={(value) => handleGainChange(index, value)}
              min={0}
              max={1}
              step={0.01}
              minLabelWidth={labelWidth}
            />
            <Spacer.Vertical size={12} />
          </View>
        ))}
        <Spacer.Vertical size={32} />
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  content: {
    paddingBottom: 24,
  },
  description: {
    color: colors.gray,
    fontSize: 14,
    lineHeight: 20,
  },
  meta: {
    color: colors.main,
    fontSize: 13,
  },
  sectionTitle: {
    color: colors.white,
    fontSize: 16,
    fontWeight: '600',
  },
});

export default ChannelMergerSplitter;
