export type SoundName = 'kick' | 'hihat' | 'clap' | 'downbeat' | 'regularbeat';

export type SoundRow = {
  name: SoundName;
  steps: boolean[];
};

export type Sounds = SoundRow[];
