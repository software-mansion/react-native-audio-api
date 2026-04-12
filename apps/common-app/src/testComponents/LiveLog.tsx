import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors, layout } from '../styles';
import EmptyState from './EmptyState';
import SectionTitle from './SectionTitle';

interface LiveLogProps {
  entries: string[];
  emptyMessage: string;
  title?: string;
}

const LiveLog: FC<LiveLogProps> = ({
  entries,
  emptyMessage,
  title = 'Live Log',
}) => (
  <View style={styles.sectionContent}>
    <SectionTitle title={title} />
    {entries.length === 0 ? (
      <EmptyState message={emptyMessage} />
    ) : (
      <View style={styles.logCard}>
        {entries.map((entry, index) => (
          <Text key={`${entry}-${index}`} style={styles.logLine}>
            {entry}
          </Text>
        ))}
      </View>
    )}
  </View>
);

const styles = StyleSheet.create({
  sectionContent: {
    gap: 12,
  },
  logCard: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 8,
    padding: 16,
  },
  logLine: {
    color: colors.gray,
    fontSize: 12,
    lineHeight: 18,
  },
});

export default LiveLog;
