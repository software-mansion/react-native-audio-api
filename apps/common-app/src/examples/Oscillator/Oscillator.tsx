import React, { useRef, useState, useEffect, FC } from 'react';
import { StyleSheet, Text, View, Pressable, Image } from 'react-native';
import {
  AudioContext,
  GainNode,
  OscillatorNode,
  StereoPannerNode,
} from 'react-native-audio-api';
import type { OscillatorType, AudioBuffer, ConvolverNode } from 'react-native-audio-api';

import { Container, Slider, Spacer, Button } from '../../components';
import { layout, colors } from '../../styles';

const INITIAL_FREQUENCY = 440;
const INITIAL_DETUNE = 0;
const INITIAL_GAIN = 1.0;
const INITIAL_PAN = 0;
const OSCILLATOR_TYPES = ['sine', 'square', 'sawtooth', 'triangle'] as const;

const labelWidth = 80;

const Oscillator: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [gain, setGain] = useState(INITIAL_GAIN);
  const [frequency, setFrequency] = useState(INITIAL_FREQUENCY);
  const [detune, setDetune] = useState(INITIAL_DETUNE);
  const [pan, setPan] = useState(INITIAL_PAN);
  const [oscillatorType, setOscillatorType] = useState<OscillatorType>('sine');

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const oscillatorRef2 = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const panRef = useRef<StereoPannerNode | null>(null);
  const convolverRef = useRef<ConvolverNode | null>(null);

  useEffect(() => {
    const fetchImpulseResponse = async () => {
      if (!audioContextRef.current) {
        audioContextRef.current = new AudioContext();
      }

        const length = audioContextRef.current.sampleRate * 2; // 2 seconds
        const impulse = audioContextRef.current.createBuffer(2, length, audioContextRef.current.sampleRate);
        
        for (let channel = 0; channel < impulse.numberOfChannels; channel++) {
          const channelData = impulse.getChannelData(channel);
          for (let i = 0; i < length; i++) {
            // Exponentially decay the impulse
            channelData[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / length, 2);
          }
        }

        convolverRef.current = audioContextRef.current?.createConvolver();
        convolverRef.current.buffer = impulse;
      }

    fetchImpulseResponse();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  const setup = (reverb: boolean) => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    oscillatorRef.current = audioContextRef.current.createOscillator();
    oscillatorRef2.current = audioContextRef.current.createOscillator();
    oscillatorRef.current.frequency.value = frequency;
    oscillatorRef2.current.frequency.value = frequency;
    oscillatorRef.current.detune.value = detune;
    oscillatorRef.current.type = oscillatorType;

    gainRef.current = audioContextRef.current.createGain();
    gainRef.current.gain.value = gain;

    panRef.current = audioContextRef.current.createStereoPanner();
    panRef.current.pan.value = pan;

    oscillatorRef.current.connect(gainRef.current);
    gainRef.current.connect(panRef.current);
    if (convolverRef.current && reverb) {
      panRef.current.connect(convolverRef.current);
      convolverRef.current.connect(audioContextRef.current.destination);
    } else {
      panRef.current.connect(audioContextRef.current.destination);
    }
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

  const handlePlayPause = (reverb: boolean) => {
    if (isPlaying) {
      oscillatorRef.current?.stop(0);
    } else {
      setup(reverb);
      oscillatorRef.current?.start(0);
      oscillatorRef.current?.stop(audioContextRef.current?.currentTime!! + 2);
      gainRef.current?.gain.setValueAtTime(0, audioContextRef.current?.currentTime!! + 2);
      oscillatorRef2.current?.start(audioContextRef.current?.currentTime!! + 2);
      oscillatorRef2.current?.connect(gainRef.current!);
      oscillatorRef2.current?.stop(audioContextRef.current?.currentTime!! + 3);
    }

    setIsPlaying((prev) => !prev);
  };

  const handleOscillatorTypeChange = (type: OscillatorType) => {
    setOscillatorType(type);
    if (oscillatorRef.current) {
      oscillatorRef.current.type = type;
    }
  };

  return (
    <Container centered>
      <Button onPress={() => handlePlayPause(false)} title={isPlaying ? 'Pause' : 'Play without reverb'} />
      <Button onPress={() => handlePlayPause(true)} title={isPlaying ? 'Pause' : 'Play reverb'} />
      <Spacer.Vertical size={49} />
      <Slider
        label="Gain"
        value={gain}
        onValueChange={handleGainChange}
        min={0.0}
        max={1.0}
        step={0.01}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Pan"
        value={pan}
        onValueChange={handlePanChange}
        min={-1}
        max={1}
        step={0.1}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Frequency"
        value={frequency}
        onValueChange={handleFrequencyChange}
        min={120}
        max={1200}
        step={10}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="Detune"
        value={detune}
        onValueChange={handleDetuneChange}
        min={0}
        step={1}
        max={100}
        minLabelWidth={labelWidth}
      />
      <Spacer.Vertical size={40} />
      <View style={styles.oscillatorTypeContainer}>
        {OSCILLATOR_TYPES.map((type: OscillatorType) => (
          <Pressable
            key={type}
            style={({ pressed }) => [
              styles.oscillatorButton,
              pressed
                ? styles.pressedOscillatorButton
                : type === oscillatorType
                  ? styles.activeOscillatorButton
                  : styles.inactiveOscillatorButton,
            ]}
            onPress={() => handleOscillatorTypeChange(type)}>
            <Text
              style={[
                styles.oscillatorButtonText,
                type === oscillatorType && styles.activeOscillatorButtonText,
              ]}>
              {type}
            </Text>
          </Pressable>
        ))}
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  oscillatorTypeContainer: {
    flexDirection: 'row',
  },
  oscillatorButton: {
    padding: layout.spacing,
    marginHorizontal: 5,
    borderWidth: 1,
    borderRadius: layout.radius,
  },
  activeOscillatorButton: {
    backgroundColor: colors.main,
    borderColor: colors.main,
  },
  pressedOscillatorButton: {
    backgroundColor: `${colors.main}88`,
    borderColor: colors.main,
  },
  inactiveOscillatorButton: {
    borderColor: colors.border,
  },
  oscillatorButtonText: {
    color: colors.white,
    textTransform: 'capitalize',
  },
  activeOscillatorButtonText: {
    color: colors.white,
  },
});

export default Oscillator;
