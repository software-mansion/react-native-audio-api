import React, { useRef, useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import { VerticalSlider } from '../../components';
import { ConvolverNode, AudioNode, AudioContext, AudioBuffer, GainNode } from 'react-native-audio-api';
import { makeReverbCurve } from './curves';

interface ReverbPedalProps {
  context: AudioContext;
  inputNode: AudioNode;
  outputNode: AudioNode;
}

export default function ReverbPedal({
  context,
  inputNode,
  outputNode,
}: ReverbPedalProps) {
  const [isActive, setIsActive] = useState(false);
  const [level, setLevel] = useState(0.5);

  const convolverNodeRef = useRef<ConvolverNode | null>(null);

  useEffect(() => {
    if (inputNode == null || outputNode == null) {
      return;
    }
    if (isActive) {
      applyEffect(context, inputNode, outputNode);
    } else {
      discardEffect(inputNode, outputNode);
    }
  }, [isActive, inputNode, outputNode]);

  useEffect(() => {
    if (isActive && inputNode && outputNode) {
      applyEffect(context, inputNode, outputNode);
    }
  }, [level]);

  const applyEffect = (context: AudioContext,inputNode: AudioNode, outputNode: AudioNode) => {
    // Select convolver based on level
    let buffer: AudioBuffer;
    let desiredDuration: number;
    if (level < 0.33) {
      desiredDuration = 1;
    } else if (level < 0.66) {
      desiredDuration = 2;
    } else {
      desiredDuration = 4;
    }
    if (convolverNodeRef.current?.buffer) {
      if (convolverNodeRef.current.buffer.duration === desiredDuration) {
        return; // No change needed
      }
      convolverNodeRef.current.disconnect();
      inputNode.disconnect(convolverNodeRef.current);
    }
    if (level < 0.33) {
      buffer = makeReverbCurve(1, context);
    } else if (level < 0.66) {
      buffer = makeReverbCurve(2, context);
    } else {
      buffer = makeReverbCurve(4, context);
    }
    const convolver = context.createConvolver();
    convolver.buffer = buffer;
    convolverNodeRef.current = convolver;
    inputNode.connect(convolver).connect(outputNode);
    inputNode.disconnect(outputNode);
    (outputNode as GainNode).gain.value = 2.5;
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    inputNode.connect(outputNode);
    (outputNode as GainNode).gain.value = 1; // Reset output gain
    if (convolverNodeRef.current) {
      inputNode.disconnect(convolverNodeRef.current);
      convolverNodeRef.current.disconnect();
    }
    convolverNodeRef.current = null;
  };

  const togglePower = () => {
    setIsActive((prev) => !prev);
  };

  return (
    <View style={styles.pedalBody}>
      <View style={styles.header}>
        <Text style={styles.brand}>RN AUDIO API</Text>
        <Text style={styles.model}>REVERB</Text>
      </View>

      <View style={styles.controlsRow}>
        <VerticalSlider
          label="LEVEL"
          value={level}
          labelColor='#fff'
          valueColor='#ffda'
          onValueChange={setLevel}
          possibleValues={[0, 0.5, 1]}
        />
      </View>

      <View style={styles.footer}>
        <View style={styles.switchContainer}>
          <View
            style={[
              styles.led,
              {
                backgroundColor: isActive ? '#ff0000' : '#330000',
              },
            ]}
          />
          <GestureDetector
            gesture={Gesture.Tap()
              .runOnJS(true)
              .onEnd(togglePower)}
          >
            <View style={[styles.stompSwitch]}>
              <View style={styles.stompInner} />
            </View>
          </GestureDetector>
          <Text style={styles.switchLabel}>
            {isActive ? 'ON' : 'BYPASS'}
          </Text>
        </View>
      </View>
    </View>
  );
}

const styles = StyleSheet.create({
  pedalBody: {
    flex: 1,
    backgroundColor: '#4368a3',
    margin: 10,
    borderRadius: 20,
    borderWidth: 4,
    borderColor: '#283f63',
    padding: 10,
    justifyContent: 'space-between',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 10 },
    shadowOpacity: 0.5,
    shadowRadius: 10,
    elevation: 10,
  },
  header: {
    alignItems: 'center',
    marginTop: 20,
  },
  brand: {
    fontSize: 16,
    fontWeight: 'bold',
    color: '#fff',
    letterSpacing: 2,
  },
  model: {
    fontSize: 32,
    fontWeight: '900',
    color: '#fff',
    fontStyle: 'italic',
  },
  controlsRow: {
    flexDirection: 'row',
    justifyContent: 'space-around',
    alignItems: 'center',
  },
  footer: {
    alignItems: 'center',
    marginBottom: 40,
  },
  switchContainer: {
    alignItems: 'center',
    gap: 10,
  },
  led: {
    width: 15,
    height: 15,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#000',
    marginBottom: 10,
    shadowColor: '#f00',
    shadowOffset: { width: 0, height: 0 },
    shadowOpacity: 0.8,
    shadowRadius: 5,
  },
  stompSwitch: {
    width: 30,
    height: 30,
    borderRadius: 30,
    backgroundColor: '#c0c0c0',
    borderWidth: 2,
    borderColor: '#888',
    justifyContent: 'center',
    alignItems: 'center',
    elevation: 5,
  },
  stompInner: {
    width: 20,
    height: 20,
    borderRadius: 20,
    backgroundColor: '#e0e0e0',
    borderWidth: 1,
    borderColor: '#aaa',
  },
  switchLabel: {
    fontWeight: 'bold',
    color: '#fff',
  },
});
