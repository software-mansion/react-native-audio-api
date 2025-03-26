import React, { FC, useEffect, useRef } from 'react';
import { View, Pressable, StyleSheet } from 'react-native';
import { AudioContext, AudioBuffer, GainNode, AudioBufferSourceNode } from 'react-native-audio-api';

const WhiteNoise: FC = () => {
  const aCtxRef = useRef<AudioContext | null>(null);
  const bufferRef = useRef<AudioBuffer | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const bufferNodeRef = useRef<AudioBufferSourceNode | null>(null);

  const createPinkNoise = () => {
    const aCtx = aCtxRef.current;

    if (!aCtx) {
      return undefined;
    }

    const bufferSize = 2 * aCtx.sampleRate;
    const output = new Float32Array(bufferSize);

    let b0, b1, b2, b3, b4, b5, b6;
    b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0;

    for (let i = 0; i < bufferSize; i += 1) {
      const white = Math.random() * 2 - 1;

      b0 = 0.99886 * b0 + white * 0.0555179;
      b1 = 0.99332 * b1 + white * 0.0750759;
      b2 = 0.969 * b2 + white * 0.153852;
      b3 = 0.8665 * b3 + white * 0.3104856;
      b4 = 0.55 * b4 + white * 0.5329522;
      b5 = -0.7616 * b5 - white * 0.016898;

      output[i] = 0.11 * (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362);
      b6 = white * 0.115926;
    }

    const noiseBuffer = aCtx.createBuffer(1, bufferSize, aCtx.sampleRate);
    noiseBuffer.copyToChannel(output, 0, 0);
    return noiseBuffer;
  }

  const onPressIn = async () => {
    if (!bufferNodeRef.current) {
      bufferNodeRef.current = await aCtxRef.current.createBufferSource();
      bufferNodeRef.current.buffer = bufferRef.current;
      bufferNodeRef.current.loop = true;
      bufferNodeRef.current.connect(gainRef.current);

      bufferNodeRef.current.start();
    }
  }

  const onPressOut = () => {
    if (bufferNodeRef.current) {
      bufferNodeRef.current.stop();
      bufferNodeRef.current = null;
    }
  }

  useEffect(() => {
    if (!aCtxRef.current) {
      aCtxRef.current = new AudioContext();
    }

    if (!bufferRef.current) {
      bufferRef.current = createPinkNoise();
    }

    if (!gainRef.current) {
      gainRef.current = aCtxRef.current.createGain();
      gainRef.current.gain.value = 0.3;
      gainRef.current.connect(aCtxRef.current.destination);
    }
  }, []);
  return (
    <View style={styles.container}>
      <Pressable onPressIn={onPressIn} onPressOut={onPressOut}>
        {({ pressed }) => (
          <View style={[styles.button, pressed && styles.buttonPressed]} />
        )}
      </Pressable>
    </View>
  );
}

export default WhiteNoise;

const styles = StyleSheet.create({
  container: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  button: {
    width: 160,
    height: 160,
    backgroundColor: '#b07eff',
    justifyContent: 'center',
    alignItems: 'center',
  },
  buttonPressed: {
    transform: [{ scale: 1.1 }],
  },
});
