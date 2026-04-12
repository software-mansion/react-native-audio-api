import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors, layout } from '../styles';

interface EmptyStateProps {
  message: string;
}

const EmptyState: FC<EmptyStateProps> = ({ message }) => (
  <View style={styles.emptyCard}>
    <Text style={styles.emptyText}>{message}</Text>
  </View>
);

const styles = StyleSheet.create({
  emptyCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    padding: 12,
  },
  emptyText: {
    color: colors.gray,
  },
});

export default EmptyState;
