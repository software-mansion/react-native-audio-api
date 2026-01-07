// filepath: /Users/michal/react-native-audio-api/apps/common-app/src/demos/PedalBoard/OverdrivePedal.tsx
import React, { useRef, useState, useEffect } from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import { VerticalSlider } from '../../components';
import { makeDistortionCurve } from './curves';
import { GainNode, BiquadFilterNode, WaveShaperNode, AudioNode, AudioContext } from 'react-native-audio-api';

const MIN_DRIVE_GAIN = 0.05;
const MAX_DRIVE_GAIN = 10;
const MIN_FREQ = 500;
const MAX_FREQ = 20000;

interface OverdrivePedalProps {
  context: AudioContext;
  inputNode: AudioNode;
  outputNode: AudioNode;
}

export default function OverdrivePedal({
  context,
  inputNode,
  outputNode,
}: OverdrivePedalProps) {
  const [isActive, setIsActive] = useState(false);
  const [drive, setDrive] = useState(0.5);
  const [tone, setTone] = useState(0.5);
  const [level, setLevel] = useState(0.5);

  const driveRef = useRef<GainNode | null>(null);
  const toneRef = useRef<BiquadFilterNode | null>(null);
  const levelRef = useRef<GainNode | null>(null);
  const shaperRef = useRef<WaveShaperNode | null>(null);

  useEffect(() => {
    initialize(context);
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
    updateAudioParams(drive, tone, level);
  }, [drive, tone, level]);

  const initialize = (context: AudioContext) => {
    driveRef.current = context.createGain();
    toneRef.current = context.createBiquadFilter();
    levelRef.current = context.createGain();
    shaperRef.current = context.createWaveShaper();
    shaperRef.current.curve = makeDistortionCurve(50, context.sampleRate);
    shaperRef.current.oversample = '4x';
    toneRef.current.type = 'lowpass';
    toneRef.current.Q.value = 1;
    updateAudioParams(drive, tone, level);
  };

  const updateAudioParams = (d: number, t: number, l: number) => {
    if (!driveRef.current || !toneRef.current || !levelRef.current) {
      return;
    }

    const driveGain = MIN_DRIVE_GAIN + d * (MAX_DRIVE_GAIN - MIN_DRIVE_GAIN);
    driveRef.current.gain.value = driveGain;

    // logarithmic mapping for tone
    const freq = MIN_FREQ * Math.pow(MAX_FREQ / MIN_FREQ, t);
    toneRef.current.frequency.value = freq;

    levelRef.current.gain.value = l;
  };

  const applyEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    if (!driveRef.current || !toneRef.current || !levelRef.current || !shaperRef.current) {
      console.warn('OverdrivePedal: Audio nodes not initialized');
      return;
    }
    inputNode
      .connect(driveRef.current!)
      .connect(shaperRef.current!)
      .connect(toneRef.current!)
      .connect(levelRef.current!)
      .connect(outputNode);
    inputNode.disconnect(outputNode);
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    inputNode.disconnect(driveRef.current!);
    inputNode.connect(outputNode);
  };

  const togglePower = () => {

    setIsActive((prev) => !prev);
  };

  return (
    <View style={styles.pedalBody}>
      <View style={styles.header}>
        <Text style={styles.brand}>RN AUDIO API</Text>
        <Text style={styles.model}>OVERDRIVE</Text>
      </View>

      <View style={styles.controlsRow}>
        <VerticalSlider
          label="DRIVE"
          value={drive}
          onValueChange={setDrive}
        />
        <VerticalSlider
          label="TONE"
          value={tone}
          onValueChange={setTone}
        />
        <VerticalSlider
          label="LEVEL"
          value={level}
          onValueChange={setLevel}
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
    backgroundColor: '#e6b800',
    margin: 10,
    borderRadius: 20,
    borderWidth: 4,
    borderColor: '#b38f00',
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
    color: '#333',
    letterSpacing: 2,
  },
  model: {
    fontSize: 32,
    fontWeight: '900',
    color: '#000',
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
    color: '#333',
  },
});
