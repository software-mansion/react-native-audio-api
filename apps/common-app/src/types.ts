export type SoundName = 'kick' | 'hi-hat' | 'clap' | 'downbeat' | 'regularbeat';

export type SoundRow = {
  name: SoundName;
  steps: boolean[];
};

export type Sounds = SoundRow[];
