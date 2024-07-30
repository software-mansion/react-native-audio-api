/* eslint-disable react/react-in-jsx-scope */
import { Button, Platform, StyleSheet, Text, View } from 'react-native';
import { useRef, useState, useEffect } from 'react';
import { Slider } from '@miblanchard/react-native-slider';
import { Kick } from './sound-engines/Kick';

import {
  AudioContext,
  type OscillatorNode,
  type GainNode,
  type StereoPannerNode,
} from 'react-native-audio-context';

const App: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState<boolean>(false);
  const [gain, setGain] = useState<number>(1.0);
  const [frequency, setFrequency] = useState<number>(440);
  const [detune, setDetune] = useState<number>(0);
  const [pan, setPan] = useState<number>(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const panRef = useRef<StereoPannerNode | null>(null);
  const kickRef = useRef<Kick | null>(null);

  const setUp = (audioContext: AudioContext) => {
    oscillatorRef.current = audioContext.createOscillator();
    oscillatorRef.current.frequency.value = frequency;
    oscillatorRef.current.detune.value = detune;
    oscillatorRef.current.type = 'sine';

    gainRef.current = audioContext.createGain();
    gainRef.current.gain.value = gain;

    panRef.current = audioContext.createStereoPanner();
    panRef.current.pan.value = pan;

    if (Platform.OS === 'android') {
      const destination = audioContext.destination;
      oscillatorRef.current.connect(gainRef.current);
      gainRef.current.connect(panRef.current);
      panRef.current.connect(destination!);
    }
  };

  const handleGainChange = (value: number[]) => {
    const newValue = value[0] || 0.0;
    setGain(newValue);
    if (gainRef.current) {
      gainRef.current.gain.value = newValue;
    }
  };

  const handlePanChange = (value: number[]) => {
    const newValue = value[0] || 0;
    setPan(newValue);
    if (panRef.current) {
      panRef.current.pan.value = newValue;
    }
  };

  const handleFrequencyChange = (value: number[]) => {
    const newValue = value[0] || 440;
    setFrequency(newValue);
    if (oscillatorRef.current) {
      oscillatorRef.current.frequency.value = newValue;
    }
  };

  const handleDetuneChange = (value: number[]) => {
    const newValue = value[0] || 0;
    setDetune(newValue);
    if (oscillatorRef.current) {
      oscillatorRef.current.detune.value = newValue;
    }
  };

  useEffect(() => {
    return () => {
      oscillatorRef.current?.stop(0);
    };
  }, []);

  const handlePlayPause = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!oscillatorRef.current) {
      setUp(audioContextRef.current);
    }

    if (isPlaying) {
      oscillatorRef.current?.stop(0);
    } else {
      oscillatorRef.current?.start(0);
    }

    setIsPlaying(!isPlaying);
  };

  const handlePlayKick = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!kickRef.current) {
      kickRef.current = new Kick(audioContextRef.current);
    }

    kickRef.current.play(audioContextRef.current.getCurrentTime());
  };

  return (
    <View style={styles.mainContainer}>
      <Text style={styles.title}>React Native Oscillator</Text>
      <View style={styles.button}>
        <Button
          title={isPlaying ? 'Pause' : 'Play'}
          onPress={handlePlayPause}
        />
      </View>
      <View style={styles.container}>
        <Text>Gain: {gain.toFixed(2)}</Text>
        <Slider
          containerStyle={styles.slider}
          value={gain}
          onValueChange={handleGainChange}
          minimumValue={0.0}
          maximumValue={1.0}
          step={0.01}
        />
      </View>
      <View style={styles.container}>
        <Text>Pan: {pan.toFixed(1)}</Text>
        <Slider
          containerStyle={styles.slider}
          value={pan}
          onValueChange={handlePanChange}
          minimumValue={-1}
          maximumValue={1}
          step={0.1}
        />
      </View>
      <View style={styles.container}>
        <Text>Frequency: {frequency.toFixed(0)}</Text>
        <Slider
          containerStyle={styles.slider}
          value={frequency}
          onValueChange={handleFrequencyChange}
          minimumValue={120}
          maximumValue={1200}
          step={10}
        />
      </View>
      <View style={styles.container}>
        <Text>Detune: {detune.toFixed(0)}</Text>
        <Slider
          containerStyle={styles.slider}
          value={detune}
          onValueChange={handleDetuneChange}
          minimumValue={0}
          maximumValue={100}
          step={1}
        />
      </View>
      <View style={styles.button}>
        <Button title="Play Kick" onPress={handlePlayKick} />
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  mainContainer: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#F5FCFF',
  },
  container: {
    justifyContent: 'center',
    alignItems: 'center',
    margin: 10,
  },
  title: {
    fontSize: 20,
    textAlign: 'center',
    margin: 10,
  },
  button: {
    width: 100,
  },
  slider: {
    width: 300,
    padding: 10,
  },
});

export default App;
