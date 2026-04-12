import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';
import { statusPillBackgroundColors } from './statusStyles';
import type { ScenarioStatus } from './types';

interface StatusPillProps {
  status: ScenarioStatus;
}

const StatusPill: FC<StatusPillProps> = ({ status }) => (
  <View
    style={[
      styles.statusPill,
      { backgroundColor: statusPillBackgroundColors[status] },
    ]}
  >
    <Text style={styles.statusText}>{status.toUpperCase()}</Text>
  </View>
);

const styles = StyleSheet.create({
  statusPill: {
    borderRadius: 999,
    paddingHorizontal: 10,
    paddingVertical: 5,
  },
  statusText: {
    color: colors.white,
    fontSize: 11,
    fontWeight: '700',
  },
});

export default StatusPill;
