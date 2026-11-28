import React, { FC } from 'react';
import { StyleSheet, Text } from 'react-native';

import { Card } from '../components';
import { colors } from '../styles';
import EmptyState from './EmptyState';
import SectionTitle from './SectionTitle';

export interface LiveLogProps {
  entries: string[];
  emptyMessage: string;
}

const LiveLog: FC<LiveLogProps> = ({ entries, emptyMessage }) => (
  <>
    <SectionTitle title="Live Log" />
    {entries.length === 0 ? (
      <EmptyState message={emptyMessage} />
    ) : (
      <Card>
        {entries.map((entry, index) => (
          <Text key={index} style={styles.logLine}>
            {entry}
          </Text>
        ))}
      </Card>
    )}
  </>
);

const styles = StyleSheet.create({
  logLine: {
    color: colors.textSecondary,
    fontSize: 12,
    lineHeight: 18,
  },
});

export default LiveLog;
