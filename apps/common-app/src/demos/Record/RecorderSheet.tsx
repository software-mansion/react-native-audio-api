import React from 'react';
import { StyleSheet, View } from 'react-native';

import { ControlsSheet } from '../../components';
import RecordControls from './RecordControls';
import RecordingLane from './RecordingLane';
import RecordingTimer from './RecordingTimer';
import StatusPill from './StatusPill';
import { RecordingState } from './types';

interface RecorderSheetProps {
  state: RecordingState;
  isInterrupted: boolean;
  onRecordPress: () => void;
  onPausePress: () => void;
  onCoveredHeightChange: (height: number) => void;
}

/** The Recorder's always-visible sheet: the live take and its controls. */
const RecorderSheet: React.FC<RecorderSheetProps> = ({
  state,
  isInterrupted,
  onRecordPress,
  onPausePress,
  onCoveredHeightChange,
}) => (
  <ControlsSheet onCoveredHeightChange={onCoveredHeightChange}>
    <View style={styles.content}>
      <StatusPill state={state} isInterrupted={isInterrupted} />
      <RecordingLane state={state} isInterrupted={isInterrupted} />
      <RecordingTimer state={state} />
      <RecordControls
        state={state}
        onRecordPress={onRecordPress}
        onPausePress={onPausePress}
      />
    </View>
  </ControlsSheet>
);

export default RecorderSheet;

const styles = StyleSheet.create({
  content: {
    paddingHorizontal: 20,
    paddingTop: 16,
    paddingBottom: 20,
    gap: 14,
  },
});
