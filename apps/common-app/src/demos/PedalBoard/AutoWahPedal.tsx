import React, { useRef, useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import { VerticalSlider } from '../../components';
import { GainNode, BiquadFilterNode, AudioNode, AudioContext, OscillatorNode } from 'react-native-audio-api';

// LFO Settings
const MIN_LFO_RATE = 0.3; // Slow sweep (0.5 Hz)
const MAX_LFO_RATE = 4; // Fast wobble (8 Hz)
const MIN_DEPTH = 50;    // Narrow frequency sweep
const MAX_DEPTH = 250;   // Wide frequency sweep

// Filter Settings
const BASE_FREQ = 800;    // Center frequency for the sweep
const MIN_Q = 1;
const MAX_Q = 20;

interface AutoWahPedalProps {
  context: AudioContext;
  inputNode: AudioNode;
  outputNode: AudioNode;
}

export default function AutoWahPedal({
  context,
  inputNode,
  outputNode,
}: AutoWahPedalProps) {
  const [isActive, setIsActive] = useState(false);

  // Repurposed State Variables
  const [rate, setRate] = useState(0.5);        // LFO Speed (was Frequency)
  const [resonance, setResonance] = useState(0.5);
  const [depth, setDepth] = useState(0.5);      // LFO Width (was Sensitivity)

  // Audio Nodes
  const inputGainRef = useRef<GainNode | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const outputGainRef = useRef<GainNode | null>(null);

  // LFO specific nodes
  const lfoOscillatorRef = useRef<OscillatorNode | null>(null);
  const lfoGainRef = useRef<GainNode | null>(null);

  useEffect(() => {
    initialize(context);
    return () => {
       try {
         lfoOscillatorRef.current?.stop();
       } catch(e) {}
    };
  }, [context]);

  useEffect(() => {
    if (inputNode == null || outputNode == null) {
      return;
    }
    if (isActive) {
      applyEffect(inputNode, outputNode);
    } else {
      discardEffect(inputNode, outputNode);
    }
  }, [isActive, inputNode, outputNode]);

  useEffect(() => {
    updateAudioParams(rate, resonance, depth);
  }, [rate, resonance, depth]);

  const initialize = (context: AudioContext) => {
    inputGainRef.current = context.createGain();
    inputGainRef.current.gain.value = 1;

    outputGainRef.current = context.createGain();
    outputGainRef.current.gain.value = 1;

    // 1. Create the Filter (The "Wah")
    filterRef.current = context.createBiquadFilter();
    filterRef.current.type = 'bandpass';
    filterRef.current.frequency.value = BASE_FREQ; // Set center point

    // 2. Create the LFO (The "Automatic Foot")
    lfoOscillatorRef.current = context.createOscillator();
    lfoOscillatorRef.current.type = 'sine'; // Smooth sweep
    lfoOscillatorRef.current.start();

    // 3. Create LFO Depth Control
    lfoGainRef.current = context.createGain();

    // 4. Connect LFO -> Depth -> Filter Frequency
    if (lfoOscillatorRef.current && lfoGainRef.current && filterRef.current) {
        lfoOscillatorRef.current.connect(lfoGainRef.current);
        lfoGainRef.current.connect(filterRef.current.frequency);
    }

    updateAudioParams(rate, resonance, depth);
  };

  const updateAudioParams = (r: number, res: number, d: number) => {
    if (!filterRef.current || !lfoOscillatorRef.current || !lfoGainRef.current) {
      return;
    }

    // 1. Update LFO Speed (Rate)
    const lfoRate = MIN_LFO_RATE + r * (MAX_LFO_RATE - MIN_LFO_RATE);
    lfoOscillatorRef.current.frequency.value = lfoRate;

    // 2. Update Filter Q (Resonance)
    const q = MIN_Q + res * (MAX_Q - MIN_Q);
    filterRef.current.Q.value = q;

    // 3. Update Sweep Width (Depth)
    const sweepWidth = MIN_DEPTH + d * (MAX_DEPTH - MIN_DEPTH);
    lfoGainRef.current.gain.value = sweepWidth;
  };

  const applyEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    if (!inputGainRef.current || !filterRef.current || !outputGainRef.current) {
      return;
    }

    inputNode.connect(inputGainRef.current);
    inputGainRef.current.connect(filterRef.current);
    filterRef.current.connect(outputGainRef.current);
    outputGainRef.current.connect(outputNode);
    (outputNode as GainNode).gain.value = 7; // Boost output level to compensate for wah attenuation

    inputNode.disconnect(outputNode);
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    if (inputGainRef.current) {
      inputNode.disconnect(inputGainRef.current);
    }

    if (filterRef.current) {
      inputGainRef.current?.disconnect(filterRef.current);
    }

    inputNode.connect(outputNode);
    (outputNode as GainNode).gain.value = 1; // Boost output level to compensate for wah attenuation
  };

  const togglePower = () => {
    setIsActive((prev) => !prev);
  };

  return (
    <View style={styles.pedalBody}>
      <View style={styles.header}>
        <Text style={styles.brand}>RN AUDIO API</Text>
        <Text style={styles.model}>AUTO WAH</Text>
      </View>

      <View style={styles.controlsRow}>
        <VerticalSlider
          label="RATE"
          value={rate}
          labelColor="#fff"
          valueColor="#ffda"
          onValueChange={setRate}
        />
        <VerticalSlider
          label="DEPTH"
          value={depth}
          labelColor="#fff"
          valueColor="#ffda"
          onValueChange={setDepth}
        />
        <VerticalSlider
          label="RES"
          value={resonance}
          labelColor="#fff"
          valueColor="#ffda"
          onValueChange={setResonance}
        />
      </View>

      <View style={styles.footer}>
        <View style={styles.switchContainer}>
          <View
            style={[
              styles.led,
              {
                backgroundColor: isActive ? '#00ff00' : '#003300', // Changed LED to Green
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
    backgroundColor: '#2d8f7b', // Changed color to Teal for "LFO" vibe
    margin: 10,
    borderRadius: 20,
    borderWidth: 4,
    borderColor: '#1f6a5a',
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
