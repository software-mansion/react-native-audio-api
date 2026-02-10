import React, { useEffect, useMemo, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  AudioContextProvider,
  useAudioContext,
} from 'react-native-audio-api';

import { Button, Container, Spacer } from '../../components';

type ComponentType = 'global' | 'provider' | 'customProvider';

const filePath =
  'https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3';

const ContextUserComponent: React.FC = () => {
  const audioContext = useAudioContext();
  const [sampleBuffer, setSampleBuffer] = useState<AudioBuffer | null>(null);
  const [sourceNode, setSourceNode] = useState<AudioBufferSourceNode | null>(
    null
  );
  const sourceNodeRef = useRef<AudioBufferSourceNode | null>(null);

  useEffect(() => {
    sourceNodeRef.current = sourceNode;
  }, [sourceNode]);

  const onToggleState = async () => {
    if (audioContext.state === 'suspended') {
      await audioContext.resume();
    } else {
      await audioContext.suspend();
    }
  };

  useEffect(() => {
    async function prepareSample() {
      const buffer = await audioContext.decodeAudioData(filePath);
      setSampleBuffer(buffer);
    }

    prepareSample();
  }, [audioContext]);

  const onPlaySample = () => {
    if (!sampleBuffer) {
      return;
    }

    const node = audioContext.createBufferSource();
    node.buffer = sampleBuffer;
    node.connect(audioContext.destination);
    node.start();

    setSourceNode(node);
  };

  const onStopSample = () => {
    if (!sourceNode) {
      return;
    }

    sourceNode.stop();
    setSourceNode(null);
  };

  useEffect(() => {
    return () => {
      if (sourceNodeRef.current) {
        sourceNodeRef.current.stop();
      }
    };
  }, []);

  return (
    <>
      <Text style={style.label}>state: {audioContext.state}</Text>
      <Spacer.Vertical size={20} />
      <Button title="Toggle state" onPress={onToggleState} />
      {!!sampleBuffer && (
        <>
          <Spacer.Vertical size={20} />
          <Text style={style.label}>
            Sample buffer loaded with {sampleBuffer.length} frames
          </Text>
          <Spacer.Vertical size={20} />
          {!sourceNode ? (
            <Button title="Play sample" onPress={onPlaySample} />
          ) : (
            <Button title="Stop sample" onPress={onStopSample} />
          )}
        </>
      )}
    </>
  );
};

const ComponentWithProvider: React.FC = () => {
  return (
    <AudioContextProvider options={{ sampleRate: 16000 }}>
      <ContextUserComponent />
    </AudioContextProvider>
  );
};

const ComponentWithCustomProvider: React.FC = () => {
  const context = useMemo(() => new AudioContext({ sampleRate: 8000 }), []);

  return (
    <AudioContextProvider context={context}>
      <ContextUserComponent />
    </AudioContextProvider>
  );
};

const LFCHooks: React.FC = () => {
  const [type, setType] = useState<ComponentType | null>(null);

  const component = useMemo(() => {
    if (!type) {
      return null;
    }

    switch (type) {
      case 'global':
        return <ContextUserComponent />;
      case 'provider':
        return <ComponentWithProvider />;
      case 'customProvider':
        return <ComponentWithCustomProvider />;
    }
  }, [type]);

  return (
    <Container>
      <View style={style.buttons}>
        {!!type && <Button title="Reset" onPress={() => setType(null)} />}
        <Spacer.Vertical size={20} />
        <Button
          title="Use global audio context"
          onPress={() => setType('global')}
        />
        <Spacer.Vertical size={20} />
        <Button
          title="Use audio context from provider"
          onPress={() => setType('provider')}
        />
        <Spacer.Vertical size={20} />
        <Button
          title="Use audio context from custom provider"
          onPress={() => setType('customProvider')}
        />
      </View>
      <Spacer.Vertical size={20} />
      {component}
    </Container>
  );
};

export default LFCHooks;

const style = StyleSheet.create({
  buttons: {
    flexDirection: 'column',
    alignItems: 'stretch',
  },
  label: {
    color: 'white',
  },
});
