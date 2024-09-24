import React from 'react';
import { useRef, useState, useEffect, FC } from 'react';
import { Button, StyleSheet, Text, View, TouchableOpacity } from 'react-native';
import CustomSlider from '../../components/CustomSlider';
import {
  AudioContext,
  WaveType,
  type GainNode,
  type OscillatorNode,
  type StereoPannerNode,
} from 'react-native-audio-api';

import Container from '../../components/Container';
import { layout, colorShades } from '../../styles';

const INITIAL_FREQUENCY = 440;
const INITIAL_DETUNE = 0;
const INITIAL_GAIN = 1.0;
const INITIAL_PAN = 0;
const OSCILLATOR_TYPES = ['sine', 'square', 'sawtooth', 'triangle'];

const Oscillator: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [gain, setGain] = useState(INITIAL_GAIN);
  const [frequency, setFrequency] = useState(INITIAL_FREQUENCY);
  const [detune, setDetune] = useState(INITIAL_DETUNE);
  const [pan, setPan] = useState(INITIAL_PAN);
  const [oscillatorType, setOscillatorType] = useState<WaveType>('sine');

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const panRef = useRef<StereoPannerNode | null>(null);

  const setup = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    oscillatorRef.current = audioContextRef.current.createOscillator();
    oscillatorRef.current.frequency.value = INITIAL_FREQUENCY;
    oscillatorRef.current.detune.value = INITIAL_DETUNE;
    oscillatorRef.current.type = oscillatorType;

    gainRef.current = audioContextRef.current.createGain();
    gainRef.current.gain.value = INITIAL_GAIN;

    panRef.current = audioContextRef.current.createStereoPanner();
    panRef.current.pan.value = INITIAL_PAN;

    oscillatorRef.current.connect(gainRef.current);
    gainRef.current.connect(panRef.current);
    panRef.current.connect(audioContextRef.current.destination);
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

  const handlePlayPause = () => {
    if (isPlaying) {
      oscillatorRef.current?.stop(0);
    } else {
      setup();
      oscillatorRef.current?.start(0);
    }

    setIsPlaying((prev) => !prev);
  };

  const handleOscillatorTypeChange = (type: WaveType) => {
    setOscillatorType(type);
    if (oscillatorRef.current) {
      oscillatorRef.current.type = type;
    }
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered={true}>
      <Button title={isPlaying ? 'Pause' : 'Play'} onPress={handlePlayPause} />
      <Text>Gain: {gain.toFixed(2)}</Text>
      <CustomSlider
        style={styles.slider}
        value={gain}
        onValueChange={handleGainChange}
        minimumValue={0.0}
        maximumValue={1.0}
        step={0.01}
      />
      <Text>Pan: {pan.toFixed(1)}</Text>
      <CustomSlider
        style={styles.slider}
        value={pan}
        onValueChange={handlePanChange}
        minimumValue={-1}
        maximumValue={1}
        step={0.1}
      />
      <Text>Frequency: {frequency.toFixed(0)}</Text>
      <CustomSlider
        style={styles.slider}
        value={frequency}
        onValueChange={handleFrequencyChange}
        minimumValue={120}
        maximumValue={1200}
        step={10}
      />
      <Text>Detune: {detune.toFixed(0)}</Text>
      <CustomSlider
        style={styles.slider}
        value={detune}
        onValueChange={handleDetuneChange}
        minimumValue={0}
        maximumValue={100}
        step={1}
      />
      <View style={styles.oscillatorTypeContainer}>
        {OSCILLATOR_TYPES.map((type: WaveType) => (
          <TouchableOpacity
            key={type}
            style={[
              styles.oscillatorButton,
              type === oscillatorType && styles.activeOscillatorButton,
            ]}
            onPress={() => handleOscillatorTypeChange(type)}
          >
            <Text
              style={[
                styles.oscillatorButtonText,
                type === oscillatorType && styles.activeOscillatorButtonText,
              ]}
            >
              {type}
            </Text>
          </TouchableOpacity>
        ))}
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  slider: {
    width: 250,
    padding: layout.spacing,
  },
  oscillatorTypeContainer: {
    flexDirection: 'row',
    marginTop: 20,
  },
  oscillatorButton: {
    padding: layout.spacing,
    marginHorizontal: 5,
    borderWidth: 1,
    borderColor: colorShades.blue.base,
    borderRadius: layout.radius,
  },
  activeOscillatorButton: {
    backgroundColor: colorShades.blue.base,
  },
  oscillatorButtonText: {
    color: colorShades.black.base,
  },
  activeOscillatorButtonText: {
    color: colorShades.white.base,
  },
});

export default Oscillator;
