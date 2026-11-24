import React, { useEffect, useState } from 'react';
import { AudioContext } from 'react-native-audio-api';

import { Knob } from '../../components';
import PedalShell, { PedalProps } from './PedalShell';
import { glide, usePedalRouting } from './usePedalRouting';

const PEDAL_COLOR = '#a78bfa';
const INITIAL_KNOB = 0.5;
/** The band-pass filter loses most of the signal, so the slot is boosted. */
const MAKEUP_GAIN = 7;

// The LFO sweeps the band-pass centre around BASE_FREQ by up to ±depth Hz.
const MIN_LFO_RATE = 0.3;
const MAX_LFO_RATE = 4;
const MIN_DEPTH = 50;
const MAX_DEPTH = 250;
const BASE_FREQ = 800;
const MIN_Q = 1;
const MAX_Q = 20;

const lfoRate = (rate: number) =>
  MIN_LFO_RATE + rate * (MAX_LFO_RATE - MIN_LFO_RATE);
const sweepWidth = (depth: number) =>
  MIN_DEPTH + depth * (MAX_DEPTH - MIN_DEPTH);
const filterQ = (resonance: number) => MIN_Q + resonance * (MAX_Q - MIN_Q);

function createAutoWah(context: AudioContext) {
  const filter = context.createBiquadFilter();
  filter.type = 'bandpass';
  filter.frequency.value = BASE_FREQ;
  filter.Q.value = filterQ(INITIAL_KNOB);

  const lfo = context.createOscillator();
  lfo.frequency.value = lfoRate(INITIAL_KNOB);

  const lfoDepth = context.createGain();
  lfoDepth.gain.value = sweepWidth(INITIAL_KNOB);

  return { chain: [filter] as const, lfo, lfoDepth };
}

export default function AutoWahPedal(props: PedalProps) {
  const { context } = props;
  const [{ chain, lfo, lfoDepth }] = useState(() => createAutoWah(context));
  const [filter] = chain;
  const power = usePedalRouting(props, chain, MAKEUP_GAIN);

  useEffect(() => {
    lfo.connect(lfoDepth);
    lfoDepth.connect(filter.frequency);
    lfo.start();
    return () => {
      lfo.stop();
      lfo.disconnect();
      lfoDepth.disconnect();
    };
  }, [lfo, lfoDepth, filter]);

  return (
    <PedalShell name="AUTO WAH" color={PEDAL_COLOR} {...power}>
      <Knob
        label="Rate"
        value={INITIAL_KNOB}
        onValueChange={(value) => glide(context, lfo.frequency, lfoRate(value))}
        color={PEDAL_COLOR}
      />
      <Knob
        label="Depth"
        value={INITIAL_KNOB}
        onValueChange={(value) =>
          glide(context, lfoDepth.gain, sweepWidth(value))
        }
        color={PEDAL_COLOR}
      />
      <Knob
        label="Res"
        value={INITIAL_KNOB}
        onValueChange={(value) => glide(context, filter.Q, filterQ(value))}
        color={PEDAL_COLOR}
      />
    </PedalShell>
  );
}
