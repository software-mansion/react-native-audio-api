import React, { useRef, useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import { ConvolverNode, AudioNode, AudioContext } from 'react-native-audio-api';
import { makeEchoCurve } from './curves';

interface EchoPedalProps {
  context: AudioContext;
  inputNode: AudioNode;
  outputNode: AudioNode;
}

export default function EchoPedal({
  context,
  inputNode,
  outputNode,
}: EchoPedalProps) {
  const [isActive, setIsActive] = useState(false);

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

  const applyEffect = (context: AudioContext, inputNode: AudioNode, outputNode: AudioNode) => {
    if (convolverNodeRef.current) {
      inputNode.disconnect(convolverNodeRef.current);
      convolverNodeRef.current.disconnect();
    }

    // Create new convolver with echo curve
    const convolver = new ConvolverNode(context, { disableNormalization: true})
    convolver.buffer = makeEchoCurve(0.7, context);
    convolverNodeRef.current = convolver;

    // Reconnect audio graph
    inputNode.connect(convolver).connect(outputNode);
    inputNode.disconnect(outputNode);
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    inputNode.connect(outputNode);
    if (convolverNodeRef.current) {
      convolverNodeRef.current.disconnect();
      inputNode.disconnect(convolverNodeRef.current);
    }
  };

  const togglePower = () => {
    setIsActive((prev) => !prev);
  };

  return (
    <View style={styles.pedalBody}>
      <View style={styles.header}>
        <Text style={styles.brand}>RN AUDIO API</Text>
        <Text style={styles.model}>ECHO</Text>
      </View>

      <View style={[styles.controlsRow, { height: 150 }]}>
      </View>

      <View style={styles.footer}>
        <View style={styles.switchContainer}>
          <View
            style={[
              styles.led,
              {
                backgroundColor: isActive ? '#00ff00' : '#003300',
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
    backgroundColor: '#2d8659',
    margin: 10,
    borderRadius: 20,
    borderWidth: 4,
    borderColor: '#1a5c3a',
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
    shadowColor: '#0f0',
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
