import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors, layout } from '../styles';

interface CurrentStepCardProps {
  message: string;
  label?: string;
}

const CurrentStepCard: FC<CurrentStepCardProps> = ({
  message,
  label = 'Current step',
}) => (
  <View style={styles.currentStepCard}>
    <Text style={styles.currentStepLabel}>{label}</Text>
    <Text style={styles.currentStepText}>{message}</Text>
  </View>
);

const styles = StyleSheet.create({
  currentStepCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 8,
    marginHorizontal: 12,
    marginBottom: 12,
    marginTop: 12,
    padding: 8,
  },
  currentStepLabel: {
    color: colors.gray,
    fontSize: 12,
    textTransform: 'uppercase',
  },
  currentStepText: {
    color: colors.white,
    fontSize: 14,
    lineHeight: 20,
  },
});

export default CurrentStepCard;
