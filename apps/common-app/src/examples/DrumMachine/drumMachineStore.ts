import { useSyncExternalStore } from 'react';

import type { Pattern } from '../../types';
import presets from './presets';

export type SoundSource = 'samples' | 'synth';

/** Shown in the preset picker once the user edits a preset's pattern. */
export const customPresetName = 'Custom';

const defaultPreset = 'Empty';

export interface DrumMachineState {
  preset: string;
  bpm: number;
  patterns: Pattern[];
  isPlaying: boolean;
  soundSource: SoundSource;
  /** Height of the controls sheet, so the sequencer can centre itself above it. */
  controlsHeight: number;
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
    controlsHeight: 0,
  };
}

let state = createInitialState();
const listeners = new Set<() => void>();

function update(changes: Partial<DrumMachineState>) {
  state = { ...state, ...changes };
  listeners.forEach((listener) => listener());
}

/**
 * State shared by the Drum Machine screen and its controls sheet. They are
 * separate navigation routes, so a component-level context can't span both.
 */
export const drumMachineStore = {
  subscribe(listener: () => void) {
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  },
  getState: () => state,
  reset: () => update(createInitialState()),

  setBpm: (bpm: number) => update({ bpm }),
  setSoundSource: (soundSource: SoundSource) => update({ soundSource }),
  setControlsHeight: (controlsHeight: number) => update({ controlsHeight }),
  togglePlayback: () => update({ isPlaying: !state.isPlaying }),

  selectPreset(name: string) {
    const preset = presets[name];
    if (!preset) {
      return;
    }

    update({
      preset: name,
      bpm: preset.bpm,
      patterns: clonePatterns(preset.pattern),
    });
  },

  toggleStep(patternIndex: number, stepIndex: number) {
    const patterns = clonePatterns(state.patterns);
    const steps = patterns[patternIndex].steps;
    steps[stepIndex] = !steps[stepIndex];

    update({ preset: customPresetName, patterns });
  },
};

export function useDrumMachineState(): DrumMachineState {
  return useSyncExternalStore(
    drumMachineStore.subscribe,
    drumMachineStore.getState
  );
}
