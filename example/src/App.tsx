import React, { useCallback, useEffect, useRef, useState } from 'react';
import { StyleSheet, View, Button, TextInput, Text } from 'react-native';
import { AudioContext, type OscillatorNode } from 'react-native-audio-context';

const BASE_FREQUENCY = 440;

const App: React.FC = () => {
  const [frequency, setFrequency] = useState(BASE_FREQUENCY);
  const osc = useRef<OscillatorNode | null>(null);
  const osc2 = useRef<OscillatorNode | null>(null);

  useEffect(() => {
    if (!osc.current) {
      const context = new AudioContext();
      osc.current = context.createOscillator(+frequency);
      osc2.current = context.createOscillator(+frequency);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    if (osc.current) {
      osc.current.frequency = frequency;
    }
  }, [frequency]);

  const start = useCallback(() => {
    osc.current?.start();
  }, []);

  const stop = useCallback(() => {
    osc.current?.stop();
  }, []);

  return (
    <View style={styles.container}>
      <Button title="Start" onPress={start} />
      <Button title="Stop" onPress={stop} />
      <TextInput
        value={frequency.toString()}
        onChangeText={(text) => setFrequency(+text)}
      />
      <Text>Current frequency: {frequency}</Text>
    </View>
  );
};

export default App;

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingHorizontal: 20,
  },
  keys: {
    fontSize: 14,
    color: 'grey',
  },
  title: {
    fontSize: 16,
    color: 'black',
    marginRight: 10,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  textInput: {
    flex: 1,
    marginVertical: 20,
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: 'black',
    borderRadius: 5,
    padding: 10,
  },
});
