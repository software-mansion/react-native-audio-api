import React, { FC } from 'react';
import { Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';

import { Button } from '../../components';
import { colors, layout } from '../../styles';
import type { LogEntry } from './log';
import { monospace, useCompact } from './ui';

interface LogViewProps {
  entries: LogEntry[];
  onToggleCompact: () => void;
  onClear: () => void;
}

const LogView: FC<LogViewProps> = ({ entries, onToggleCompact, onClear }) => {
  const isCompact = useCompact();
  return (
    <View style={[styles.container, isCompact && styles.compactContainer]}>
      <View style={styles.header}>
        <Text style={isCompact ? styles.compactTitle : styles.title}>Log</Text>
        <View style={styles.headerActions}>
          <Pressable
            testID={isCompact ? 'playground-detailed' : 'playground-compact'}
            onPress={onToggleCompact}
            style={({ pressed }) => [
              styles.headerChip,
              pressed && styles.pressed,
            ]}
          >
            <Text style={styles.headerChipText}>
              {isCompact ? 'Detailed' : 'Compact'}
            </Text>
          </Pressable>
          {isCompact ? (
            <Pressable
              testID="log-clear"
              onPress={onClear}
              style={({ pressed }) => [
                styles.headerChip,
                styles.clearChip,
                pressed && styles.pressed,
              ]}
            >
              <Text style={styles.clearChipText}>Clear</Text>
            </Pressable>
          ) : (
            <Button testID="log-clear" title="Clear" onPress={onClear} />
          )}
        </View>
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
};

export default LogView;

const styles = StyleSheet.create({
  container: {
    height: 240,
    padding: layout.spacing,
    gap: layout.spacing,
    backgroundColor: colors.backgroundLight,
  },
  compactContainer: {
    height: 88,
    padding: 4,
    gap: 2,
  },
  header: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  headerActions: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 6,
  },
  title: {
    color: colors.white,
    fontWeight: '600',
  },
  compactTitle: {
    color: colors.white,
    fontSize: 11,
    fontWeight: '700',
  },
  headerChip: {
    paddingVertical: 3,
    paddingHorizontal: 8,
    borderRadius: layout.radius,
    borderWidth: 1,
    borderColor: colors.border,
    backgroundColor: colors.backgroundDark,
  },
  clearChip: {
    borderColor: colors.main,
    backgroundColor: colors.main,
  },
  pressed: {
    opacity: 0.65,
  },
  headerChipText: {
    color: colors.gray,
    fontSize: 11,
  },
  clearChipText: {
    color: colors.white,
    fontSize: 11,
    fontWeight: '600',
  },
  lines: {
    flex: 1,
  },
  line: {
    color: colors.gray,
    fontSize: 11,
    fontFamily: monospace,
  },
});
