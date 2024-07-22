import React from 'react';
import { Button, Platform, StyleSheet, Text, View } from 'react-native';
import { useRef, useEffect } from 'react';

import { AudioContext, type Oscillator } from 'react-native-audio-context';

const App: React.FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<Oscillator | null>(null);
  const secondaryOscillatorRef = useRef<Oscillator | null>(null);

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();

      oscillatorRef.current = audioContextRef.current.createOscillator();

      secondaryOscillatorRef.current =
        audioContextRef.current.createOscillator();
      secondaryOscillatorRef.current.frequency = 840;
      secondaryOscillatorRef.current.type = 'square';

      // Destination is not implemented on IOS yet
      if (Platform.OS === 'android') {
        const destination = audioContextRef.current.destination;
        oscillatorRef.current.connect(destination!);
        secondaryOscillatorRef.current.connect(destination!);
      }
    }

    return () => {
      //TODO
    };
  }, []);

  const startOscillator = () => {
    oscillatorRef.current?.start(0);
    secondaryOscillatorRef.current?.start(0);
  };
  const stopOscillator = () => {
    oscillatorRef.current?.stop(0);
    secondaryOscillatorRef.current?.stop(0);
  };

  return (
    <View style={styles.container}>
      <Text style={styles.title}>React Native Oscillator</Text>
      <Button title="Start Oscillator" onPress={startOscillator} />
      <Button title="Stop Oscillator" onPress={stopOscillator} />
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
    backgroundColor: '#F5FCFF',
  },
  title: {
    fontSize: 20,
    textAlign: 'center',
    margin: 10,
  },
});

export default App;
