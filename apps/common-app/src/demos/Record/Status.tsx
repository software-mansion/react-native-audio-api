import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { RecordingState } from './types';

interface StatusProps {
  state: RecordingState;
}

const Status: FC<StatusProps> = ({ state }) => {
  let statusText = '';

  if (state === RecordingState.Idle) {
    statusText = 'Idle';
  } else if (state === RecordingState.Loading) {
    statusText = 'Loading...';
  } else if (state === RecordingState.Recording) {
    statusText = 'Recording...';
  } else if (state === RecordingState.Paused) {
    statusText = 'Paused';
  } else if (state === RecordingState.ReadyToPlay) {
    statusText = 'Ready to Play';
  } else if (state === RecordingState.Playing) {
    statusText = 'Playing...';
  }

  return (
    <View style={styles.statusView}>
      <Text style={styles.statusText}>{statusText}</Text>
    </View>
  );
};

export default Status;

const styles = StyleSheet.create({
  statusView: {
    marginTop: 40,
    alignItems: 'center',
  },
  statusText: {
    fontSize: 18,
    color: 'white',
  },
});
