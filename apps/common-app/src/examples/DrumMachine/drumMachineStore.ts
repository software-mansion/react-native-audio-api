import type { Pattern } from '../../utils/usePlayer';
import { createStore } from '../../utils/createStore';
import presets from './presets';

export type SoundSource = 'samples' | 'synth';

/** Shown in the preset picker once the user edits a preset's pattern. */
const customPresetName = 'Custom';

const defaultPreset = 'Empty';

interface DrumMachineState {
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

const store = createStore(createInitialState());

/** State shared by the Drum Machine screen and its controls sheet. */
export const drumMachineStore = {
  reset: () => store.setState(createInitialState()),

  setBpm: (bpm: number) => store.setState({ bpm }),
  setSoundSource: (soundSource: SoundSource) => store.setState({ soundSource }),
  setControlsHeight: (controlsHeight: number) =>
    store.setState({ controlsHeight }),
  togglePlayback: () =>
    store.setState({ isPlaying: !store.getState().isPlaying }),

  selectPreset(name: string) {
    const preset = presets[name];
    if (!preset) {
      return;
    }

    store.setState({
      preset: name,
      bpm: preset.bpm,
      patterns: clonePatterns(preset.pattern),
    });
  },

  toggleStep(patternIndex: number, stepIndex: number) {
    const patterns = clonePatterns(store.getState().patterns);
    const steps = patterns[patternIndex].steps;
    steps[stepIndex] = !steps[stepIndex];

    store.setState({ preset: customPresetName, patterns });
  },
};

export const useDrumMachineState = store.useStore;
