import React, { FC, ReactNode, useState } from 'react';

import FilterList from '@site/src/ui/FilterList';
import Switch from '@site/src/ui/Switch';

import AudioGraph, { AudioGraphSpec, GraphNodeSpec } from './AudioGraph';
import { DEFAULT_SAMPLE_SOURCES, SAMPLE_SOURCES, sampleSourceOptions, SampleSourceId } from './sources';
import styles from './styles.module.css';
import { EffectChain, useEffectChain, UseEffectChainOptions } from './useEffectChain';

interface TransportState {
  ctx: unknown;
  enabled: boolean;
  isPlaying: boolean;
  isLoading: boolean;
  togglePlayback(): void;
  toggleEffect(): void;
}

interface EffectPlaygroundHeaderProps {
  chain: TransportState;
  spec: AudioGraphSpec;
}

const EffectPlaygroundHeader: FC<EffectPlaygroundHeaderProps> = ({ chain, spec }) => {
  const clickTargets = spec.effects.map((effect, index) => (
    <React.Fragment key={effect.label}>
      {index > 0 && ' or '}
      <code>{effect.label}</code>
    </React.Fragment>
  ));

  return (
    <div className={styles.header}>
      <div className={styles.transportRow}>
        <button
          type="button"
          className={styles.transportButton}
          onClick={chain.togglePlayback}
          disabled={!chain.ctx}
          aria-label={chain.isPlaying ? 'Stop playback' : 'Start playback'}>
          {chain.isLoading ? 'Loading…' : chain.isPlaying ? 'Stop' : 'Play'}
        </button>
        <p className={styles.hint}>
          Click the {clickTargets} box to bypass {spec.effects.length > 1 ? 'them' : 'it'} and compare.
        </p>
      </div>
      <AudioGraph
        spec={spec}
        enabled={chain.enabled}
        isPlaying={chain.isPlaying}
        isLoading={chain.isLoading}
        onEffectClick={chain.toggleEffect}
        onSourceClick={chain.togglePlayback}
      />
    </div>
  );
};

export interface UseEffectPlaygroundOptions<TParams> extends Omit<UseEffectChainOptions<TParams>, 'source'> {
  initialSource: SampleSourceId;
  /** Samples offered in the picker, in order; defaults to music and voice. */
  sources?: SampleSourceId[];
  /** The node(s) the reader can bypass, in signal order. */
  effectNodes: GraphNodeSpec[];
  feedbackNode?: GraphNodeSpec;
  /** Name used by the bypass switch when it covers more than one node. */
  groupLabel?: string;
}

export interface EffectPlayground<TParams> {
  chain: EffectChain<TParams>;
  header: ReactNode;
  /** Sample picker and bypass switch, meant to open every effect's control panel. */
  commonControls: ReactNode;
  /** Name of the decoded buffer for the code snippet, matching the selected sample. */
  sampleVariable: string;
}

/**
 * Shared scaffolding of every effect playground: the sample choice, the audio chain, the clickable
 * graph in the header and the controls that all effects have in common.
 */
export function useEffectPlayground<TParams>({
  initialSource,
  sources = DEFAULT_SAMPLE_SOURCES,
  effectNodes,
  feedbackNode,
  groupLabel,
  ...chainOptions
}: UseEffectPlaygroundOptions<TParams>): EffectPlayground<TParams> {
  const [source, setSource] = useState<SampleSourceId>(initialSource);
  const chain = useEffectChain({ ...chainOptions, source });

  const spec: AudioGraphSpec = {
    effects: effectNodes,
    feedback: feedbackNode,
    dryPath: Boolean(chainOptions.mixesDrySignal),
  };
  const switchLabel = groupLabel ?? effectNodes[0].label;

  const header = <EffectPlaygroundHeader chain={chain} spec={spec} />;

  const commonControls = (
    <>
      <FilterList<SampleSourceId>
        ariaLabel="Audio sample"
        label="Audio sample"
        options={sampleSourceOptions(sources)}
        value={source}
        onChange={setSource}
      />
      <Switch
        ariaLabel={`Enable ${switchLabel}`}
        checked={chain.enabled}
        onChange={chain.setEnabled}
        rightLabel={`${switchLabel} enabled`}
      />
    </>
  );

  return {
    chain,
    header,
    commonControls,
    sampleVariable: SAMPLE_SOURCES[source].codeVariable,
  };
}
