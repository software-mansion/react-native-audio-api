import React, { useRef, useEffect } from 'react';
import { Button, StyleSheet, Text, View } from 'react-native';

import { AudioContext, type Oscillator } from 'react-native-audio-context';

function SimplePlayer() {
  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<Oscillator | null>(null);
  const secondaryOscillatorRef = useRef<Oscillator | null>(null);

  useEffect(() => {
    audioContextRef.current = new AudioContext();
    oscillatorRef.current = audioContextRef.current.createOscillator();
    secondaryOscillatorRef.current = audioContextRef.current.createOscillator();
    secondaryOscillatorRef.current.frequency = 300;
    secondaryOscillatorRef.current.type = 'square';

    const destination = audioContextRef.current.destination();
    oscillatorRef.current.connect(destination);
    secondaryOscillatorRef.current.connect(destination);

    return () => {
      //TODO
    };
  }, []);

  const startOscillator = () => {
    oscillatorRef.current?.start();
    secondaryOscillatorRef.current?.start();
  };
  const stopOscillator = () => {
    oscillatorRef.current?.stop();
    secondaryOscillatorRef.current?.stop();
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>React Native Oscillator</Text>
      <Button title="Start Oscillator" onPress={startOscillator} />
      <Button title="Stop Oscillator" onPress={stopOscillator} />
    </View>
  );
}

export default SimplePlayer;

const styles = StyleSheet.create({
  container: {
    backgroundColor: '#F5FCFF',
  },
  title: {
    fontSize: 20,
    textAlign: 'center',
    margin: 10,
  },
});
