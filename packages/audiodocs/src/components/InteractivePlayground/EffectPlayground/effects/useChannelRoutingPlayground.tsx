import React, { useMemo, useState } from 'react';

import FilterList from '@site/src/ui/FilterList';

import { buildEffectCode } from '../playgroundCode';
import styles from '../styles.module.css';
import type { EffectFactory } from '../types';
import { useEffectPlayground } from '../useEffectPlayground';
import { StereoMeterPreview } from '../visualizers/previews';

type ChannelTarget = 'left' | 'right' | 'both' | 'none';

interface ChannelRoutingParams {
  left: ChannelTarget;
  right: ChannelTarget;
}

/** Swapped by default so the routing is audible the moment the reader presses play. */
const initialState: ChannelRoutingParams = { left: 'right', right: 'left' };

const TARGET_OPTIONS: Array<{ value: ChannelTarget; label: string }> = [
  { value: 'left', label: 'Left speaker' },
  { value: 'right', label: 'Right speaker' },
  { value: 'both', label: 'Both' },
  { value: 'none', label: 'Nowhere' },
];

const MERGER_INPUTS: Record<ChannelTarget, number[]> = { left: [0], right: [1], both: [0, 1], none: [] };
const CHANNEL_NAMES = ['left', 'right'] as const;
const SPEAKER_NAMES = ['left', 'right'];

interface BrowserBackedContext {
  context: globalThis.AudioContext;
}

interface BrowserBackedNode {
  node: globalThis.AudioNode;
}

/**
 * The splitter and merger are created on the browser context behind the wrapper because the
 * react-native-audio-api release bundled with the docs predates their web implementations. The
 * pass-through gains keep the chain wired through the library's own nodes.
 */
const createChannelRoutingEffect: EffectFactory<ChannelRoutingParams> = (ctx, params) => {
  const input = ctx.createGain();
  const output = ctx.createGain();
  const browserContext = (ctx as unknown as BrowserBackedContext).context;
  const splitter = browserContext.createChannelSplitter(2);
  const merger = browserContext.createChannelMerger(2);

  (input as unknown as BrowserBackedNode).node.connect(splitter);
  merger.connect((output as unknown as BrowserBackedNode).node);

  const route = (next: ChannelRoutingParams) => {
    splitter.disconnect();
    CHANNEL_NAMES.forEach((channel, outputIndex) => {
      for (const inputIndex of MERGER_INPUTS[next[channel]]) {
        splitter.connect(merger, outputIndex, inputIndex);
      }
    });
  };

  route(params);

  return {
    input,
    output,
    update: route,
    dispose: () => {
      input.disconnect();
      splitter.disconnect();
      merger.disconnect();
      output.disconnect();
    },
  };
};

function describeTarget(target: ChannelTarget): string {
  return target === 'none' ? 'muted' : target;
}

function connectionLines(params: ChannelRoutingParams): string {
  return CHANNEL_NAMES.map((channel, outputIndex) => {
    const inputs = MERGER_INPUTS[params[channel]];
    if (inputs.length === 0) {
      return `// the ${channel} channel is left disconnected`;
    }
    return inputs
      .map(
        (inputIndex) =>
          `splitter.connect(merger, ${outputIndex}, ${inputIndex}); // ${channel} channel → ${SPEAKER_NAMES[inputIndex]} speaker`
      )
      .join('\n');
  }).join('\n');
}

export function useChannelRoutingPlayground() {
  const [left, setLeft] = useState<ChannelTarget>(initialState.left);
  const [right, setRight] = useState<ChannelTarget>(initialState.right);
  const params = useMemo<ChannelRoutingParams>(() => ({ left, right }), [left, right]);

  const { chain, header, commonControls, sampleVariable } = useEffectPlayground({
    createEffect: createChannelRoutingEffect,
    params,
    initialSource: 'stereo',
    sources: ['stereo', 'music', 'voice'],
    effectNodes: [
      { label: 'ChannelSplitterNode', sublabel: 'stereo in · 2 mono outputs' },
      { label: 'ChannelMergerNode', sublabel: `L → ${describeTarget(left)} · R → ${describeTarget(right)}` },
    ],
    groupLabel: 'Channel routing',
  });

  const code = buildEffectCode({
    sampleVariable,
    effectLabel: 'Channel routing',
    enabled: chain.enabled,
    setup: `const splitter = ctx.createChannelSplitter(2);
const merger = ctx.createChannelMerger(2);`,
    wiring: `source.connect(splitter);
${connectionLines(params)}
merger.connect(ctx.destination);`,
    bypassedWiring: `// splitter and merger are bypassed: the stereo signal plays unchanged
source.connect(ctx.destination);`,
  });

  const controls = (
    <div className={styles.controlsPanel}>
      {commonControls}
      <p className={styles.controlCaption}>Left channel goes to</p>
      <FilterList<ChannelTarget>
        ariaLabel="Left channel destination"
        label="Left channel goes to"
        options={TARGET_OPTIONS}
        value={left}
        onChange={setLeft}
      />
      <p className={styles.controlCaption}>Right channel goes to</p>
      <FilterList<ChannelTarget>
        ariaLabel="Right channel destination"
        label="Right channel goes to"
        options={TARGET_OPTIONS}
        value={right}
        onChange={setRight}
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
