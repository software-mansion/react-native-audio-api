import React from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { colors, layout } from '../../../styles';
import type { TranscriptEntry } from '../voiceSession';

export function Transcript({ entries }: { entries: TranscriptEntry[] }) {
  return (
    <ScrollView
      contentContainerStyle={styles.transcript}
      style={styles.transcriptScroll}
    >
      {entries.length === 0 ? (
        <Text style={styles.placeholder}>
          Connect to begin a voice conversation.
        </Text>
      ) : (
        entries.map((entry) => (
          <View key={entry.id} style={styles.entry}>
            <Text style={styles.role}>
              {entry.role === 'user'
                ? 'You'
                : entry.interrupted
                  ? 'Assistant · interrupted'
                  : 'Assistant'}
            </Text>
            <Text style={styles.entryText}>{entry.text}</Text>
          </View>
        ))
      )}
    </ScrollView>
  );
}

const styles = StyleSheet.create({
  transcriptScroll: {
    flex: 1,
  },
  transcript: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: layout.spacing * 2,
    minHeight: 180,
    padding: layout.spacing * 2,
  },
  placeholder: {
    color: colors.gray,
  },
  entry: {
    gap: 2,
  },
  role: {
    color: colors.main,
    fontSize: 12,
    fontWeight: '700',
    textTransform: 'uppercase',
  },
  entryText: {
    color: colors.white,
    fontSize: 15,
    lineHeight: 21,
  },
});
