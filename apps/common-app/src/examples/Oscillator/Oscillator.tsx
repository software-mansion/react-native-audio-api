import React, { FC, useRef, useState } from 'react';
import { StyleSheet } from 'react-native';
import type {
  GainNode,
  OscillatorNode,
  OscillatorType,
  StereoPannerNode,
} from 'react-native-audio-api';

import {
  Button,
  Container,
  SegmentedControl,
  Slider,
  type SegmentedControlOption,
} from '../../components';
import { useAudioContext } from '../../utils/audio';

const INITIAL_FREQUENCY = 440;
const INITIAL_DETUNE = 0;
const INITIAL_GAIN = 1.0;
const INITIAL_PAN = 0;

const oscillatorTypeOptions: SegmentedControlOption<OscillatorType>[] = [
  { value: 'sine', label: 'Sine' },
  { value: 'square', label: 'Square' },
  { value: 'sawtooth', label: 'Saw' },
  { value: 'triangle', label: 'Triangle' },
];

const Oscillator: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [gain, setGain] = useState(INITIAL_GAIN);
  const [frequency, setFrequency] = useState(INITIAL_FREQUENCY);
  const [detune, setDetune] = useState(INITIAL_DETUNE);
  const [pan, setPan] = useState(INITIAL_PAN);
  const [oscillatorType, setOscillatorType] = useState<OscillatorType>('sine');

  const audioContext = useAudioContext();
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const panRef = useRef<StereoPannerNode | null>(null);

  const start = () => {
    const oscillator = audioContext.createOscillator();
    oscillator.frequency.value = frequency;
    oscillator.detune.value = detune;
    oscillator.type = oscillatorType;

    const gainNode = audioContext.createGain();
    gainNode.gain.value = gain;

    const panner = audioContext.createStereoPanner();
    panner.pan.value = pan;

    oscillator
      .connect(gainNode)
      .connect(panner)
      .connect(audioContext.destination);
    oscillator.start(0);

    oscillatorRef.current = oscillator;
    gainRef.current = gainNode;
    panRef.current = panner;
  };

  const stop = () => {
    oscillatorRef.current?.stop(0);
    oscillatorRef.current?.disconnect();
    gainRef.current?.disconnect();
    panRef.current?.disconnect();
    oscillatorRef.current = null;
    gainRef.current = null;
    panRef.current = null;
  };

  const handlePlayPause = () => {
    if (isPlaying) {
      stop();
    } else {
      start();
    }
    setIsPlaying(!isPlaying);
  };

  const handleGainChange = (newValue: number) => {
    setGain(newValue);
    if (gainRef.current) {
      gainRef.current.gain.value = newValue;
    }
  };

  const handlePanChange = (newValue: number) => {
    setPan(newValue);
    if (panRef.current) {
      panRef.current.pan.value = newValue;
    }
  };

  const handleFrequencyChange = (newValue: number) => {
    setFrequency(newValue);
    if (oscillatorRef.current) {
      oscillatorRef.current.frequency.value = newValue;
    }
  };

  const handleDetuneChange = (newValue: number) => {
    setDetune(newValue);
    if (oscillatorRef.current) {
      oscillatorRef.current.detune.value = newValue;
    }
  };

  const handleOscillatorTypeChange = (type: OscillatorType) => {
    setOscillatorType(type);
    if (oscillatorRef.current) {
      oscillatorRef.current.type = type;
    }
  };

  return (
    <Container style={styles.content}>
      <Button onPress={handlePlayPause} title={isPlaying ? 'Pause' : 'Play'} />
      <Slider
        label="Gain"
        value={gain}
        onValueChange={handleGainChange}
        min={0.0}
        max={1.0}
        step={0.01}
      />
      <Slider
        label="Pan"
        value={pan}
        onValueChange={handlePanChange}
        min={-1}
        max={1}
        step={0.1}
      />
      <Slider
        label="Frequency"
        value={frequency}
        onValueChange={handleFrequencyChange}
        min={120}
        max={1200}
        step={10}
      />
      <Slider
        label="Detune"
        value={detune}
        onValueChange={handleDetuneChange}
        min={0}
        step={1}
        max={100}
      />
      <SegmentedControl
        options={oscillatorTypeOptions}
        value={oscillatorType}
        onChange={handleOscillatorTypeChange}
      />
    </Container>
  );
};

const styles = StyleSheet.create({
  content: {
    justifyContent: 'center',
    gap: 24,
  },
});

export default Oscillator;
