import React, { useState } from 'react';
import { AudioContext } from 'react-native-audio-api';

import { Knob } from '../../components';
import { makeDistortionCurve } from './curves';
import PedalShell, { PedalProps } from './PedalShell';
import { glide, usePedalRouting } from './usePedalRouting';

const PEDAL_COLOR = '#ffb020';
const INITIAL_KNOB = 0.5;

const MIN_DRIVE_GAIN = 0.05;
const MAX_DRIVE_GAIN = 10;
const MIN_FREQ = 500;
const MAX_FREQ = 20000;

const driveGain = (drive: number) =>
  MIN_DRIVE_GAIN + drive * (MAX_DRIVE_GAIN - MIN_DRIVE_GAIN);
const toneFrequency = (tone: number) =>
  MIN_FREQ * Math.pow(MAX_FREQ / MIN_FREQ, tone);

function createOverdrive(context: AudioContext) {
  const drive = context.createGain();
  drive.gain.value = driveGain(INITIAL_KNOB);

  const shaper = context.createWaveShaper();
  shaper.curve = makeDistortionCurve(50, context.sampleRate);
  shaper.oversample = '4x';

  const tone = context.createBiquadFilter();
  tone.type = 'lowpass';
  tone.Q.value = 1;
  tone.frequency.value = toneFrequency(INITIAL_KNOB);

  const level = context.createGain();
  level.gain.value = INITIAL_KNOB;

  return [drive, shaper, tone, level] as const;
}

export default function OverdrivePedal(props: PedalProps) {
  const { context } = props;
  const [chain] = useState(() => createOverdrive(context));
  const [drive, , tone, level] = chain;
  const power = usePedalRouting(props, chain);

  return (
    <PedalShell name="OVERDRIVE" color={PEDAL_COLOR} {...power}>
      <Knob
        label="Drive"
        value={INITIAL_KNOB}
        onValueChange={(value) => glide(context, drive.gain, driveGain(value))}
        color={PEDAL_COLOR}
      />
      <Knob
        label="Tone"
        value={INITIAL_KNOB}
        onValueChange={(value) =>
          glide(context, tone.frequency, toneFrequency(value))
        }
        color={PEDAL_COLOR}
      />
      <Knob
        label="Level"
        value={INITIAL_KNOB}
        onValueChange={(value) => glide(context, level.gain, value)}
        color={PEDAL_COLOR}
      />
    </PedalShell>
  );
}
