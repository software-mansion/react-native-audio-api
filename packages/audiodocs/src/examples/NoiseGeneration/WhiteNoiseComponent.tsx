import React, { FC, useEffect, useRef } from 'react';
import { View, Pressable, StyleSheet } from 'react-native';
import { AudioContext, AudioBuffer, GainNode, AudioBufferSourceNode } from 'react-native-audio-api';

const WhiteNoise: FC = () => {
  const aCtxRef = useRef<AudioContext | null>(null);
  const bufferRef = useRef<AudioBuffer | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const bufferNodeRef = useRef<AudioBufferSourceNode | null>(null);

  const createWhiteNoise = () => {
    const aCtx = aCtxRef.current;

    if (!aCtx) {
      return undefined;
    }

    const bufferSize = 2 * aCtx.sampleRate;
    const output = new Float32Array(bufferSize);

    for (let i = 0; i < bufferSize; i += 1) {
      output[i] = Math.random() * 2 - 1;
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
      bufferRef.current = createWhiteNoise();
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
