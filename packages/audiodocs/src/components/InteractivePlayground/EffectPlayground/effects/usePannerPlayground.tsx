import React, { useMemo, useState } from 'react';
import type { AudioContext, GainNode } from 'react-native-audio-api';

import FilterList from '@site/src/ui/FilterList';
import SliderInput from '@site/src/ui/SliderInput';

import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { useEffectPlayground } from '../useEffectPlayground';
import { StereoMeterPreview } from '../visualizers/previews';

type DistanceModel = 'linear' | 'inverse' | 'exponential';

interface PannerParams {
  positionX: number;
  positionZ: number;
  distanceModel: DistanceModel;
  rolloffFactor: number;
}

const initialState: PannerParams = {
  positionX: -3,
  positionZ: -1,
  distanceModel: 'inverse',
  rolloffFactor: 1,
};

const MAX_DISTANCE = 10;
const POSITION_SMOOTHING_SECONDS = 0.02;

const distanceModelOptions: Array<{ value: DistanceModel; label: string }> = [
  { value: 'inverse', label: 'Inverse' },
  { value: 'linear', label: 'Linear' },
  { value: 'exponential', label: 'Exponential' },
];

/**
 * The web wrapper bundled with the docs predates PannerNode, so the playground builds the browser
 * node directly and splices it between two wrapper gains that the chain can connect to.
 */
interface BrowserBackedContext {
  context: globalThis.AudioContext;
}

interface BrowserBackedNode {
  node: globalThis.AudioNode;
}

function browserNodeOf(node: GainNode): globalThis.AudioNode {
  return (node as unknown as BrowserBackedNode).node;
}

function applyPannerParams(panner: globalThis.PannerNode, params: PannerParams, ctx: AudioContext): void {
  panner.distanceModel = params.distanceModel;
  panner.rolloffFactor = params.rolloffFactor;
  panner.positionX.setTargetAtTime(params.positionX, ctx.currentTime, POSITION_SMOOTHING_SECONDS);
  panner.positionZ.setTargetAtTime(params.positionZ, ctx.currentTime, POSITION_SMOOTHING_SECONDS);
}

function describePosition(x: number, z: number): string {
  const side = Math.abs(x) < 0.5 ? 'center' : x < 0 ? 'left' : 'right';
  const facing = z > 0 ? 'behind' : 'front';
  return `${side}, ${facing}`;
}

const createPannerEffect: EffectFactory<PannerParams> = (ctx, params) => {
  const input = ctx.createGain();
  const output = ctx.createGain();
  const browserContext = (ctx as unknown as BrowserBackedContext).context;
  const panner = browserContext.createPanner();

  panner.panningModel = 'equalpower';
  panner.maxDistance = MAX_DISTANCE;
  panner.positionX.value = params.positionX;
  panner.positionZ.value = params.positionZ;
  applyPannerParams(panner, params, ctx);

  browserNodeOf(input).connect(panner);
  panner.connect(browserNodeOf(output));

  return {
    input,
    output,
    update: (next) => applyPannerParams(panner, next, ctx),
    dispose: () => {
      input.disconnect();
      panner.disconnect();
      output.disconnect();
    },
  };
};

export function usePannerPlayground() {
  const [positionX, setPositionX] = useState(initialState.positionX);
  const [positionZ, setPositionZ] = useState(initialState.positionZ);
  const [distanceModel, setDistanceModel] = useState<DistanceModel>(initialState.distanceModel);
  const [rolloffFactor, setRolloffFactor] = useState(initialState.rolloffFactor);
  const params = useMemo<PannerParams>(
    () => ({ positionX, positionZ, distanceModel, rolloffFactor }),
    [positionX, positionZ, distanceModel, rolloffFactor]
  );

  const distance = Math.hypot(positionX, positionZ);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createPannerEffect,
    params,
    initialSource: 'voice',
    effectNodes: [
      {
        label: 'PannerNode',
        sublabel: `${distance.toFixed(1)} m · ${describePosition(positionX, positionZ)}`,
      },
    ],
  });

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'PannerNode',
    enabled: chain.enabled,
    setup: `const panner = ctx.createPanner();
panner.panningModel = 'equalpower';
panner.distanceModel = '${distanceModel}';
panner.rolloffFactor = ${rolloffFactor.toFixed(1)};
panner.maxDistance = ${MAX_DISTANCE};
// The listener sits at the origin facing -Z: negative x is left, negative z is in front
panner.positionX.value = ${positionX.toFixed(1)};
panner.positionY.value = 0;
panner.positionZ.value = ${positionZ.toFixed(1)};`,
    wiring: `source.connect(panner);
panner.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <SliderInput
        label="Position X (left / right)"
        value={positionX}
        min={-MAX_DISTANCE}
        max={MAX_DISTANCE}
        step={0.1}
        unit="m"
        onChange={setPositionX}
      />
      <SliderInput
        label="Position Z (front / behind)"
        value={positionZ}
        min={-MAX_DISTANCE}
        max={MAX_DISTANCE}
        step={0.1}
        unit="m"
        onChange={setPositionZ}
      />
      <FilterList<DistanceModel>
        ariaLabel="Distance model"
        label="Distance model"
        options={distanceModelOptions}
        value={distanceModel}
        onChange={setDistanceModel}
      />
      <SliderInput
        label="Rolloff factor"
        value={rolloffFactor}
        min={0}
        max={distanceModel === 'linear' ? 1 : 4}
        step={0.1}
        onChange={setRolloffFactor}
      />
    </div>
  );

  return {
    code,
    controls,
    header,
    example: StereoMeterPreview,
    props: { ctx: chain.ctx, monitor: chain.monitor },
  };
}
