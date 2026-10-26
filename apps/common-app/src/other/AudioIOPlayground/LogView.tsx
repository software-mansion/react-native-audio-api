import React, { FC } from 'react';
import { Platform, ScrollView, StyleSheet, Text, View } from 'react-native';

import { Button } from '../../components';
import { colors, layout } from '../../styles';
import type { LogEntry } from './log';

interface LogViewProps {
  entries: LogEntry[];
  onClear: () => void;
}

const LogView: FC<LogViewProps> = ({ entries, onClear }) => (
  <View style={styles.container}>
    <View style={styles.header}>
      <Text style={styles.title}>Log</Text>
      <Button testID="log-clear" title="Clear" onPress={onClear} />
    </View>
    <ScrollView testID="log" style={styles.lines}>
      {entries.map((entry) => (
        <Text key={entry.id} style={styles.line}>
          {entry.text}
        </Text>
      ))}
    </ScrollView>
  </View>
);

export default LogView;

const styles = StyleSheet.create({
  container: {
    height: 240,
    padding: layout.spacing,
    gap: layout.spacing,
    backgroundColor: colors.backgroundLight,
  },
  header: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
  },
  title: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '600',
  },
  lines: {
    flex: 1,
  },
  line: {
    color: colors.gray,
    fontSize: 11,
    fontFamily: Platform.select({ ios: 'Menlo', default: 'monospace' }),
  },
});
