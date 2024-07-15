import React, { useEffect, useRef } from 'react';
import { StyleSheet, View, Button } from 'react-native';
import { AudioContext, type OscillatorType } from 'react-native-audio-context';

const App: React.FC = () => {
  const osc1 = useRef<OscillatorType | null>(null);
  const osc2 = useRef<OscillatorType | null>(null);

  useEffect(() => {
    if (!osc2.current) {
      const context = new AudioContext();
      osc1.current = context.createOscillator(440);
      osc2.current = context.createOscillator(880);
    }
  }, []);

  const start = () => {
    osc1.current?.start();
  };

  const stop = () => {
    osc1.current?.stop();
  };

  const start2 = () => {
    osc2.current?.start();
  };

  const stop2 = () => {
    osc2.current?.stop();
  };

  return (
    <View style={styles.container}>
      <Button title="Start1" onPress={start} />
      <Button title="Stop1" onPress={stop} />
      <Button title="Start1" onPress={start2} />
      <Button title="Stop1" onPress={stop2} />
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
