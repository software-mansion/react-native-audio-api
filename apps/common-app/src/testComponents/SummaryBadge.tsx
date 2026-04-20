import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors, layout } from '../styles';

interface SummaryBadgeProps {
  label: string;
  value: string;
}

const SummaryBadge: FC<SummaryBadgeProps> = ({ label, value }) => (
  <View style={styles.summaryBadge}>
    <Text style={styles.summaryLabel}>{label}</Text>
    <Text style={styles.summaryValue}>{value}</Text>
  </View>
);

const styles = StyleSheet.create({
  summaryBadge: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    flex: 1,
    gap: 2,
    padding: 8,
  },
  summaryLabel: {
    color: colors.gray,
    fontSize: 10,
    textTransform: 'uppercase',
  },
  summaryValue: {
    color: colors.white,
    fontSize: 14,
    fontWeight: '700',
  },
});

export default SummaryBadge;
