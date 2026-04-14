import React, { FC } from 'react';
import { StyleSheet } from 'react-native';

import Animated from 'react-native-reanimated';
import PauseButton from './PauseButton';
import RecordButton from './RecordButton';
import { RecordingState } from './types';

interface ControlPanelProps {
  state: RecordingState;
  onToggleState: (action: RecordingState) => void;
}

const ControlPanel: FC<ControlPanelProps> = ({ state, onToggleState }) => {
  return (
    <Animated.View style={styles.controlPanelView}>
      <PauseButton
        state={state}
        onPress={() => {
          onToggleState(RecordingState.Paused);
        }}
      />
      <RecordButton state={state} onToggleState={onToggleState} />
    </Animated.View>
  );
};

export default ControlPanel;

const styles = StyleSheet.create({
  controlPanelView: {
    alignItems: 'center',
    justifyContent: 'center',
    flexDirection: 'row',
    gap: 12,
  },
});
