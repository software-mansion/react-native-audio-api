import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { ScrollView, StyleSheet, Text } from 'react-native';
import {
  AudioBufferSourceNode,
  AudioManager,
  AudioNode,
  GainNode,
} from 'react-native-audio-api';

import { Button, Card, Container, Slider } from '../../components';
import { colors, typography } from '../../styles';
import {
  activatePlaybackSession,
  useAudioContext,
  useDecodedAudio,
} from '../../utils/audio';

const SURROUND_5_1 =
  'https://www2.iis.fraunhofer.de/AAC/ChID-BLITS-EBU-Narration.mp4';

const CHANNEL_COUNT = 6;
const CHANNEL_LABELS = ['L', 'R', 'C', 'LFE', 'Ls', 'Rs'] as const;

const INITIAL_GAINS = [0.3, 0.6, 0.1, 1.0, 0.7, 0.7];

/**
 * Graph: AudioBufferSourceNode (5.1) → ChannelSplitter(6) → per-channel
 * GainNodes → ChannelMerger(2) stereo fold-down → destination
 *
 * Fold-down: L/C/LFE/Ls → left, R/C/LFE/Rs → right
 */
const ChannelMergerSplitter: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [gains, setGains] = useState<number[]>([...INITIAL_GAINS]);

  const audioContext = useAudioContext();
  const buffer = useDecodedAudio(audioContext, SURROUND_5_1);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const channelGainsRef = useRef<GainNode[]>([]);
  const outputNodeRef = useRef<AudioNode | null>(null);

  const teardown = useCallback(() => {
    if (sourceRef.current) {
      sourceRef.current.onended = null;
      sourceRef.current.stop(0);
      sourceRef.current.disconnect();
    }
    outputNodeRef.current?.disconnect();

    sourceRef.current = null;
    channelGainsRef.current = [];
    outputNodeRef.current = null;
  }, []);

  const setup = async () => {
    if (!buffer) {
      return;
    }

    await activatePlaybackSession();

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    const numberOfChannels = Math.min(buffer.numberOfChannels, CHANNEL_COUNT);
    const splitter = audioContext.createChannelSplitter(numberOfChannels);
    const stereoMerger = audioContext.createChannelMerger(2);

    const source = audioContext.createBufferSource();
    source.buffer = buffer;
    source.connect(splitter);

    const channelGains: GainNode[] = [];
    for (let i = 0; i < numberOfChannels; i++) {
      const gain = audioContext.createGain();
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

    stereoMerger.connect(audioContext.destination);

    sourceRef.current = source;
    channelGainsRef.current = channelGains;
    outputNodeRef.current = stereoMerger;

    source.onended = () => {
      setIsPlaying(false);
      teardown();
    };

    source.start(0);
  };

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

  useEffect(
    () => () => {
      teardown();
      AudioManager.setAudioSessionActivity(false).catch(() => {});
    },
    [teardown]
  );

  return (
    <Container>
      <ScrollView
        contentContainerStyle={styles.content}
        showsVerticalScrollIndicator={false}
      >
        <Text style={typography.hint}>
          Plays a 5.1 surround test (L R C LFE Ls Rs) through
          AudioBufferSourceNode → ChannelSplitter → per-channel gains →
          ChannelMerger stereo fold-down. Solo a channel with its gain slider.
        </Text>
        <Text style={styles.meta}>
          {buffer
            ? `Loaded ${buffer.numberOfChannels}-channel buffer`
            : 'Loading 5.1 sample…'}
        </Text>
        <Button
          onPress={handlePlayPause}
          title={isPlaying ? 'Stop' : 'Play'}
          disabled={!buffer}
        />
        <Card style={styles.gains}>
          <Text style={typography.sectionTitle}>Channel gains</Text>
          {CHANNEL_LABELS.map((label, index) => (
            <Slider
              key={label}
              label={label}
              value={gains[index]}
              onValueChange={(value) => handleGainChange(index, value)}
              min={0}
              max={1}
              step={0.01}
            />
          ))}
        </Card>
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  content: {
    gap: 16,
    paddingBottom: 24,
  },
  meta: {
    color: colors.blue,
    fontSize: 13,
  },
  gains: {
    gap: 12,
  },
});

export default ChannelMergerSplitter;
