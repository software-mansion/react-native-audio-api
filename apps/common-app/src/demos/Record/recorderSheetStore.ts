import { createStore } from '../../utils/createStore';
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

const store = createStore<RecorderSheetState>({
  state: RecordingState.Idle,
  isInterrupted: false,
  onRecordPress: noop,
  onPausePress: noop,
  sheetHeight: 0,
});

/**
 * Connects the Recorder screen, which owns the recorder, to its controls sheet:
 * the screen publishes its state and handlers here and the sheet renders them.
 */
export const recorderSheetStore = {
  publish: (
    changes: Pick<
      RecorderSheetState,
      'state' | 'isInterrupted' | 'onRecordPress' | 'onPausePress'
    >
  ) => store.setState(changes),
  setSheetHeight: (sheetHeight: number) => store.setState({ sheetHeight }),
};

export const useRecorderSheetState = store.useStore;
