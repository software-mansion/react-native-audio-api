import React, { FC, useEffect, useRef, useState } from 'react';
import { StyleSheet } from 'react-native';
import {
  AudioBufferSourceNode,
  AudioManager,
  ConvolverNode,
  GainNode,
} from 'react-native-audio-api';

import { Button, Container, Slider } from '../../components';
import {
  activatePlaybackSession,
  useAudioContext,
  useDecodedAudio,
} from '../../utils/audio';

const IR = require('./ir.wav');
const TRACK = require('./track.mp3');

/** Skips the track's quiet intro. */
const TRACK_START_OFFSET_S = 10;

const ConvolverIR: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [gain, setGain] = useState(0.28);

  const audioContext = useAudioContext();
  const impulse = useDecodedAudio(audioContext, IR);
  const track = useDecodedAudio(audioContext, TRACK);
  const isReady = impulse !== null && track !== null;

  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const convolverRef = useRef<ConvolverNode | null>(null);

  const stopPlayback = () => {
    if (bufferSourceRef.current) {
      bufferSourceRef.current.onended = null;
      bufferSourceRef.current.stop(0);
      bufferSourceRef.current.disconnect();
    }
    gainRef.current?.disconnect();
    convolverRef.current?.disconnect();
    bufferSourceRef.current = null;
    gainRef.current = null;
    convolverRef.current = null;
    setIsPlaying(false);
  };

  useEffect(
    () => () => {
      bufferSourceRef.current?.disconnect();
      gainRef.current?.disconnect();
      convolverRef.current?.disconnect();
      AudioManager.setAudioSessionActivity(false).catch(() => {});
    },
    []
  );

  const startPlayback = async (useConvolver: boolean) => {
    if (!impulse || !track || isPlaying) {
      return;
    }

    await activatePlaybackSession();

    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    }

    const source = audioContext.createBufferSource();
    source.buffer = track;

    const gainNode = audioContext.createGain();
    gainNode.gain.value = gain;
    source.connect(gainNode);

    if (useConvolver) {
      const convolver = audioContext.createConvolver();
      convolver.buffer = impulse;
      gainNode.connect(convolver).connect(audioContext.destination);
      convolverRef.current = convolver;
    } else {
      gainNode.connect(audioContext.destination);
    }

    source.onended = stopPlayback;
    bufferSourceRef.current = source;
    gainRef.current = gainNode;

    source.start(audioContext.currentTime + 0.1, TRACK_START_OFFSET_S);
    setIsPlaying(true);
  };

  const onGainChange = (value: number) => {
    setGain(value);
    if (gainRef.current) {
      gainRef.current.gain.value = value;
    }
  };

  return (
    <Container centered style={styles.content}>
      <Button
        title="Play (convolver)"
        onPress={() => startPlayback(true).catch(() => {})}
        minWidth={200}
        disabled={!isReady || isPlaying}
      />
      <Button
        title="Play (dry)"
        onPress={() => startPlayback(false).catch(() => {})}
        minWidth={200}
        disabled={!isReady || isPlaying}
      />
      <Button
        title="Stop"
        onPress={stopPlayback}
        minWidth={200}
        disabled={!isPlaying}
      />
      <Slider
        label="Gain"
        value={gain}
        onValueChange={onGainChange}
        min={0.02}
        max={0.6}
        step={0.01}
      />
    </Container>
  );
};

export default ConvolverIR;

const styles = StyleSheet.create({
  content: {
    gap: 12,
  },
});
