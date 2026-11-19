import { useSyncExternalStore } from 'react';

import { RecordingState } from './types';

export interface RecorderSheetState {
  state: RecordingState;
  isInterrupted: boolean;
  onRecordPress: () => void;
  onPausePress: () => void;
  /** Height of the sheet, so the recordings list can scroll clear of it. */
  sheetHeight: number;
}

const noop = () => {};

let snapshot: RecorderSheetState = {
  state: RecordingState.Idle,
  isInterrupted: false,
  onRecordPress: noop,
  onPausePress: noop,
  sheetHeight: 0,
};
const listeners = new Set<() => void>();

function update(changes: Partial<RecorderSheetState>) {
  snapshot = { ...snapshot, ...changes };
  listeners.forEach((listener) => listener());
}

/**
 * Connects the Recorder screen, which owns the recorder, to its controls sheet.
 * They are separate navigation routes, so the screen publishes its state and
 * handlers here and the sheet renders them.
 */
export const recorderSheetStore = {
  subscribe(listener: () => void) {
    listeners.add(listener);
    return () => {
      listeners.delete(listener);
    };
  },
  getSnapshot: () => snapshot,

  publish: (
    changes: Pick<
      RecorderSheetState,
      'state' | 'isInterrupted' | 'onRecordPress' | 'onPausePress'
    >
  ) => update(changes),
  setSheetHeight: (sheetHeight: number) => update({ sheetHeight }),
};

export function useRecorderSheetState(): RecorderSheetState {
  return useSyncExternalStore(
    recorderSheetStore.subscribe,
    recorderSheetStore.getSnapshot
  );
}
