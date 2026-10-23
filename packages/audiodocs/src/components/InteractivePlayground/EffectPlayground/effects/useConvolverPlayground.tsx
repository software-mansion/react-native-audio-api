import React, { useMemo, useState } from 'react';
import type { AudioBuffer, AudioContext } from 'react-native-audio-api';

import SliderInput from '@site/src/ui/SliderInput';
import Switch from '@site/src/ui/Switch';

import { WaveformPreview } from '../visualizers/previews';
import { setParamSmoothly } from '../effectUtils';
import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { ANALYSER_FFT_SIZE } from '../useEffectChain';
import { useEffectPlayground } from '../useEffectPlayground';
import CurveChart from '../visualizers/CurveChart';

interface ConvolverParams {
  duration: number;
  decay: number;
  normalize: boolean;
  wet: number;
}

const initialState: ConvolverParams = { duration: 2, decay: 3, normalize: true, wet: 0.6 };

/** Sliders emit many values per second; rebuilding the impulse for each one would stutter. */
const IMPULSE_REBUILD_DELAY_MS = 80;
const ENVELOPE_CHART_POINTS = 120;

function createImpulseResponse(ctx: AudioContext, duration: number, decay: number): AudioBuffer {
  const length = Math.max(1, Math.round(ctx.sampleRate * duration));
  const impulse = ctx.createBuffer(2, length, ctx.sampleRate);

  for (let channel = 0; channel < impulse.numberOfChannels; channel++) {
    const samples = impulse.getChannelData(channel);
    for (let i = 0; i < length; i++) {
      samples[i] = (Math.random() * 2 - 1) * Math.pow(1 - i / length, decay);
    }
  }

  return impulse;
}

function impulseChanged(previous: ConvolverParams, next: ConvolverParams): boolean {
  return (
    previous.duration !== next.duration ||
    previous.decay !== next.decay ||
    previous.normalize !== next.normalize
  );
}

const createConvolverEffect: EffectFactory<ConvolverParams> = (ctx, params) => {
  const input = ctx.createGain();
  const convolver = ctx.createConvolver();
  const wetLevel = ctx.createGain();

  input.connect(convolver);
  convolver.connect(wetLevel);
  wetLevel.gain.value = params.wet;

  let appliedParams = params;
  let rebuildTimer: number | null = null;

  const applyImpulse = (next: ConvolverParams) => {
    // `normalize` is read when the buffer is assigned, so it has to be set first.
    convolver.normalize = next.normalize;
    convolver.buffer = createImpulseResponse(ctx, next.duration, next.decay);
    appliedParams = next;
  };

  applyImpulse(params);

  return {
    input,
    output: wetLevel,
    update: (next) => {
      setParamSmoothly(wetLevel.gain, next.wet, ctx);

      if (!impulseChanged(appliedParams, next)) {
        return;
      }
      if (rebuildTimer !== null) {
        window.clearTimeout(rebuildTimer);
      }
      rebuildTimer = window.setTimeout(() => {
        rebuildTimer = null;
        applyImpulse(next);
      }, IMPULSE_REBUILD_DELAY_MS);
    },
    dispose: () => {
      if (rebuildTimer !== null) {
        window.clearTimeout(rebuildTimer);
      }
      input.disconnect();
      convolver.disconnect();
      wetLevel.disconnect();
    },
  };
};

export function useConvolverPlayground() {
  const [duration, setDuration] = useState(initialState.duration);
  const [decay, setDecay] = useState(initialState.decay);
  const [normalize, setNormalize] = useState(initialState.normalize);
  const [wet, setWet] = useState(initialState.wet);

  const params = useMemo<ConvolverParams>(
    () => ({ duration, decay, normalize, wet }),
    [duration, decay, normalize, wet]
  );

  const envelope = useMemo(() => {
    const points = new Float32Array(ENVELOPE_CHART_POINTS);
    for (let i = 0; i < ENVELOPE_CHART_POINTS; i++) {
      points[i] = Math.pow(1 - i / (ENVELOPE_CHART_POINTS - 1), decay);
    }
    return points;
  }, [decay]);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createConvolverEffect,
    params,
    mixesDrySignal: true,
    initialSource: 'voice',
    effectNodes: [{ label: 'ConvolverNode', sublabel: `impulse · ${duration.toFixed(2)} s` }],
  });

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'ConvolverNode',
    enabled: chain.enabled,
    setup: `const convolver = ctx.createConvolver();
convolver.normalize = ${normalize};
convolver.buffer = createImpulseResponse(${duration.toFixed(2)}, ${decay.toFixed(1)});
const wet = ctx.createGain();
wet.gain.value = ${wet.toFixed(2)};`,
    wiring: `source.connect(ctx.destination); // dry signal
source.connect(convolver);
convolver.connect(wet);
wet.connect(ctx.destination);`,
    bypassedWiring: `// ConvolverNode is bypassed: only the dry signal reaches the destination
source.connect(ctx.destination);`,
    helpers: `// A synthetic room: white noise that fades out over \`duration\` seconds.
function createImpulseResponse(duration: number, decay: number) {
  const length = Math.round(ctx.sampleRate * duration);
  const impulse = ctx.createBuffer(2, length, ctx.sampleRate);
  for (let channel = 0; channel < 2; channel++) {
    const samples = impulse.getChannelData(channel);
    for (let i = 0; i < length; i++) {
      samples[i] = (Math.random() * 2 - 1) * (1 - i / length) ** decay;
    }
  }
  return impulse;
}`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <SliderInput
        label="Impulse duration"
        value={duration}
        min={0.1}
        max={4}
        step={0.05}
        unit="s"
        onChange={setDuration}
      />
      <SliderInput label="Decay" value={decay} min={0.5} max={8} step={0.1} onChange={setDecay} />
      <SliderInput label="Wet level" value={wet} min={0} max={1} step={0.01} onChange={setWet} />
      <Switch ariaLabel="Normalize impulse response" checked={normalize} onChange={setNormalize} rightLabel="Normalize (LOUD warning)" />
      <div className={styles.chartBlock}>
        <CurveChart values={envelope} yMin={0} yMax={1} height={90} />
        <p className={styles.chartCaption}>Impulse response envelope over its duration</p>
      </div>
    </div>
  );

  return {
    code,
    controls,
    header,
    example: WaveformPreview,
    props: { analyserNode: chain.analyser, fftSize: ANALYSER_FFT_SIZE },
  };
}
