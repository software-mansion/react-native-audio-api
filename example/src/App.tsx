import React, { useCallback, useRef, useState } from 'react';
import { StyleSheet, View, Button, TextInput } from 'react-native';
import { AudioContext, type OscillatorNode } from 'react-native-audio-context';

const App: React.FC = () => {
  const [frequency, setFrequency] = useState('440');
  const osc = useRef<OscillatorNode | null>(null);

  const start = useCallback(() => {
    if (!osc.current) {
      const context = new AudioContext();
      osc.current = context.createOscillator(+frequency);
    }

    osc.current.start();
  }, [frequency]);

  const stop = useCallback(() => {
    if (!osc.current) {
      return;
    }

    osc.current.stop();
  }, []);

  const changeFrequency = useCallback(() => {
    if (!osc.current) {
      return;
    }

    osc.current.frequency = parseFloat(frequency);
  }, [frequency]);

  const getFrequency = useCallback(() => {
    if (!osc.current) {
      return;
    }

    console.log(osc.current.frequency);

    return osc.current.frequency;
  }, []);

  return (
    <View style={styles.container}>
      <Button title="Start1" onPress={start} />
      <Button title="Stop1" onPress={stop} />
      <TextInput
        value={frequency}
        onChangeText={(text) => setFrequency(text)}
      />
      <Button title="Change frequency" onPress={changeFrequency} />
      <Button title="Get current frequency" onPress={getFrequency} />
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
