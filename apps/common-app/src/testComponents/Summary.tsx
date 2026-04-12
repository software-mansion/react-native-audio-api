import React, { FC } from 'react';
import { StyleSheet, View } from 'react-native';

import { layout } from '../styles';
import type { SummaryItem } from './types';
import SummaryBadge from './SummaryBadge';

interface SummaryProps {
  items: SummaryItem[];
}

const Summary: FC<SummaryProps> = ({ items }) => (
  <View style={styles.summaryRow}>
    {items.map((item, index) => (
      <SummaryBadge
        key={`${item.label}-${item.value}-${index}`}
        label={item.label}
        value={item.value}
      />
    ))}
  </View>
);

const styles = StyleSheet.create({
  summaryRow: {
    flexDirection: 'row',
    gap: layout.spacing,
    paddingHorizontal: 12,
  },
});

export default Summary;
