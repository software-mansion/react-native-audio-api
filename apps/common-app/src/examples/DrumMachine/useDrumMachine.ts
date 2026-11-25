import { useMemo, useState } from 'react';

import type { Pattern } from '../../utils/usePlayer';
import presets from './presets';

export type SoundSource = 'samples' | 'synth';

/** Shown in the preset picker once the user edits a preset's pattern. */
const customPresetName = 'Custom';

const defaultPreset = 'Empty';

export interface DrumMachineState {
  preset: string;
  bpm: number;
  patterns: Pattern[];
  isPlaying: boolean;
  soundSource: SoundSource;
}

function clonePatterns(patterns: Pattern[]): Pattern[] {
  return patterns.map((pattern) => ({ ...pattern, steps: [...pattern.steps] }));
}

function createInitialState(): DrumMachineState {
  return {
    preset: defaultPreset,
    bpm: presets[defaultPreset].bpm,
    patterns: clonePatterns(presets[defaultPreset].pattern),
    isPlaying: false,
    soundSource: 'samples',
  };
}

/** The Drum Machine's pattern, tempo and transport, with stable actions. */
export function useDrumMachine() {
  const [state, setState] = useState(createInitialState);

  const actions = useMemo(
    () => ({
      setBpm: (bpm: number) => setState((s) => ({ ...s, bpm })),
      setSoundSource: (soundSource: SoundSource) =>
        setState((s) => ({ ...s, soundSource })),
      togglePlayback: () =>
        setState((s) => ({ ...s, isPlaying: !s.isPlaying })),

      selectPreset: (name: string) => {
        const preset = presets[name];
        if (!preset) {
          return;
        }

        setState((s) => ({
          ...s,
          preset: name,
          bpm: preset.bpm,
          patterns: clonePatterns(preset.pattern),
        }));
      },

      toggleStep: (patternIndex: number, stepIndex: number) => {
        setState((s) => {
          const patterns = clonePatterns(s.patterns);
          const steps = patterns[patternIndex].steps;
          steps[stepIndex] = !steps[stepIndex];
          return { ...s, preset: customPresetName, patterns };
        });
      },
    }),
    []
  );

  return [state, actions] as const;
}

export type DrumMachineActions = ReturnType<typeof useDrumMachine>[1];
