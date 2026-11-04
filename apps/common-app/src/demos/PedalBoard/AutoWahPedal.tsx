import React, { useRef, useState, useEffect } from 'react';
import {
  GainNode,
  BiquadFilterNode,
  AudioNode,
  AudioContext,
  OscillatorNode,
} from 'react-native-audio-api';

import { Knob } from '../../components';
import PedalShell from './PedalShell';

const PEDAL_COLOR = '#a78bfa';

// LFO Settings
const MIN_LFO_RATE = 0.3; // Slow sweep (0.5 Hz)
const MAX_LFO_RATE = 4; // Fast wobble (8 Hz)
const MIN_DEPTH = 50; // Narrow frequency sweep
const MAX_DEPTH = 250; // Wide frequency sweep

// Filter Settings
const BASE_FREQ = 800; // Center frequency for the sweep
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
  const [rate, setRate] = useState(0.5); // LFO Speed (was Frequency)
  const [resonance, setResonance] = useState(0.5);
  const [depth, setDepth] = useState(0.5); // LFO Width (was Sensitivity)

  // Audio Nodes
  const inputGainRef = useRef<GainNode | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const outputGainRef = useRef<GainNode | null>(null);

  // LFO specific nodes
  const lfoOscillatorRef = useRef<OscillatorNode | null>(null);
  const lfoGainRef = useRef<GainNode | null>(null);

  // Whether inputNode is currently routed through the effect chain (vs bypassed)
  const isChainConnectedRef = useRef(false);

  useEffect(() => {
    initialize(context);
    return () => {
      try {
        lfoOscillatorRef.current?.stop();
      } catch (e) {}
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
  }, [isActive]);

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
    if (
      !filterRef.current ||
      !lfoOscillatorRef.current ||
      !lfoGainRef.current
    ) {
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
    isChainConnectedRef.current = true;
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    if (isChainConnectedRef.current) {
      inputNode.disconnect(inputGainRef.current!);
      inputGainRef.current!.disconnect(filterRef.current!);
      filterRef.current!.disconnect(outputGainRef.current!);
      outputGainRef.current!.disconnect(outputNode);
      isChainConnectedRef.current = false;
    }

    inputNode.connect(outputNode);
    (outputNode as GainNode).gain.value = 1; // Boost output level to compensate for wah attenuation
  };

  const togglePower = () => {
    setIsActive((prev) => !prev);
  };

  return (
    <PedalShell
      name="AUTO WAH"
      color={PEDAL_COLOR}
      isActive={isActive}
      onToggle={togglePower}
    >
      <Knob
        label="Rate"
        value={rate}
        onValueChange={setRate}
        color={PEDAL_COLOR}
      />
      <Knob
        label="Depth"
        value={depth}
        onValueChange={setDepth}
        color={PEDAL_COLOR}
      />
      <Knob
        label="Res"
        value={resonance}
        onValueChange={setResonance}
        color={PEDAL_COLOR}
      />
    </PedalShell>
  );
}
