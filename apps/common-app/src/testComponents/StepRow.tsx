import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';
import { statusTextColors } from './statusStyles';
import type { StepItem } from './types';

interface StepRowProps {
  step: StepItem;
}

const StepRow: FC<StepRowProps> = ({ step }) => (
  <View style={styles.stepRow}>
    <View style={styles.stepRowHeader}>
      <Text
        style={[styles.stepStatus, { color: statusTextColors[step.status] }]}
      >
        {step.status.toUpperCase()}
      </Text>
      <Text style={styles.stepMessage}>{step.message}</Text>
    </View>
    {step.details ? (
      <Text style={styles.stepDetails}>{step.details}</Text>
    ) : null}
  </View>
);

const styles = StyleSheet.create({
  stepRow: {
    gap: 6,
  },
  stepRowHeader: {
    alignItems: 'center',
    flexDirection: 'row',
    gap: 12,
  },
  stepStatus: {
    fontSize: 11,
    fontWeight: '700',
    minWidth: 44,
  },
  stepMessage: {
    color: colors.white,
    flex: 1,
    fontSize: 13,
  },
  stepDetails: {
    color: colors.gray,
    fontSize: 12,
    lineHeight: 17,
    paddingLeft: 56,
  },
});

export default StepRow;
