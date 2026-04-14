export enum RecordingState {
  Idle = 'Idle',
  Loading = 'Loading',
  Recording = 'Recording',
  Paused = 'Paused',
  ReadyToPlay = 'ReadyToPlay',
  Playing = 'Playing',
}

export type AudioCallback = (normalized: number) => void;
