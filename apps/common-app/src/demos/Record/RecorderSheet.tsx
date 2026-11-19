import React from 'react';
import { LayoutChangeEvent, StyleSheet, View } from 'react-native';

import RecordControls from './RecordControls';
import RecordingLane from './RecordingLane';
import RecordingTimer from './RecordingTimer';
import {
  recorderSheetStore,
  useRecorderSheetState,
} from './recorderSheetStore';
import StatusPill from './StatusPill';

const onSheetLayout = (event: LayoutChangeEvent) => {
  recorderSheetStore.setSheetHeight(event.nativeEvent.layout.height);
};

/** Content of the always-visible Recorder sheet: the live take and its controls. */
const RecorderSheet: React.FC = () => {
  const { state, isInterrupted, onRecordPress, onPausePress } =
    useRecorderSheetState();

  return (
    <View style={styles.sheet} onLayout={onSheetLayout}>
      <StatusPill state={state} isInterrupted={isInterrupted} />
      <RecordingLane state={state} isInterrupted={isInterrupted} />
      <RecordingTimer state={state} />
      <RecordControls
        state={state}
        onRecordPress={onRecordPress}
        onPausePress={onPausePress}
      />
    </View>
  );
};

export default RecorderSheet;

const styles = StyleSheet.create({
  sheet: {
    paddingHorizontal: 20,
    paddingTop: 16,
    paddingBottom: 28,
    gap: 14,
  },
});
