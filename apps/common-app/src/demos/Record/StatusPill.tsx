import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../../styles';
import { RecordingState } from './types';

interface StatusPillProps {
  state: RecordingState;
  isInterrupted: boolean;
}

const statusLabels: Partial<Record<RecordingState, string>> = {
  [RecordingState.Recording]: 'Recording',
  [RecordingState.Paused]: 'Paused',
  [RecordingState.Saving]: 'Saving',
};

/**
 * What the recorder is doing, as a capsule above the lane: red with a dot while
 * recording. Idle keeps the space empty so nothing shifts on start.
 */
const StatusPill: React.FC<StatusPillProps> = ({ state, isInterrupted }) => {
  const label = isInterrupted ? 'Interrupted' : statusLabels[state];
  const isLive = state === RecordingState.Recording && !isInterrupted;

  return (
    <View style={styles.slot}>
      {label && (
        <View style={[styles.pill, isLive && styles.livePill]}>
          {isLive && <View style={styles.dot} />}
          <Text style={[styles.label, isLive && styles.liveLabel]}>
            {label}
          </Text>
        </View>
      )}
    </View>
  );
};

export default StatusPill;

const styles = StyleSheet.create({
  slot: {
    height: 30,
    alignItems: 'center',
    justifyContent: 'center',
  },
  pill: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 7,
    paddingHorizontal: 12,
    paddingVertical: 6,
    borderRadius: 15,
    backgroundColor: colors.track,
  },
  livePill: {
    backgroundColor: 'rgba(255, 119, 116, 0.14)',
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: colors.accent,
  },
  label: {
    fontSize: 13,
    fontWeight: '600',
    color: colors.textSecondary,
  },
  liveLabel: {
    color: colors.accent,
  },
});
