// filepath: /Users/michal/react-native-audio-api/apps/common-app/src/demos/PedalBoard/OverdrivePedal.tsx
import React, { useRef, useState, useEffect } from 'react';
import {
  GainNode,
  BiquadFilterNode,
  WaveShaperNode,
  AudioNode,
  AudioContext,
} from 'react-native-audio-api';

import { Knob } from '../../components';
import { makeDistortionCurve } from './curves';
import PedalShell from './PedalShell';

const PEDAL_COLOR = '#ffb020';

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
  // Whether inputNode is currently routed through the effect chain (vs bypassed)
  const isChainConnectedRef = useRef(false);

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
  }, [isActive]);

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
    if (
      !driveRef.current ||
      !toneRef.current ||
      !levelRef.current ||
      !shaperRef.current
    ) {
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
    isChainConnectedRef.current = true;
  };

  const discardEffect = (inputNode: AudioNode, outputNode: AudioNode) => {
    if (isChainConnectedRef.current) {
      inputNode.disconnect(driveRef.current!);
      isChainConnectedRef.current = false;
    }
    inputNode.connect(outputNode);
  };

  const togglePower = () => {
    setIsActive((prev) => !prev);
  };

  return (
    <PedalShell
      name="OVERDRIVE"
      color={PEDAL_COLOR}
      isActive={isActive}
      onToggle={togglePower}
    >
      <Knob
        label="Drive"
        value={drive}
        onValueChange={setDrive}
        color={PEDAL_COLOR}
      />
      <Knob
        label="Tone"
        value={tone}
        onValueChange={setTone}
        color={PEDAL_COLOR}
      />
      <Knob
        label="Level"
        value={level}
        onValueChange={setLevel}
        color={PEDAL_COLOR}
      />
    </PedalShell>
  );
}
