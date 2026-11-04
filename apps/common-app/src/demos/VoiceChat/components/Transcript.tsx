import React, { ComponentRef, useRef } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';

import { colors } from '../../../styles';
import type { TranscriptEntry } from '../voiceSession';

interface TranscriptProps {
  entries: TranscriptEntry[];
  /** Highlights the last assistant line while its audio plays. */
  assistantSpeaking: boolean;
  /** Shown before the first turn. */
  placeholder: string;
}

const fadeHeight = 48;

/**
 * The conversation so far, newest at the bottom: your turns as tinted bubbles
 * on the right, the assistant's as plain text. Older lines fade out under the
 * top edge.
 */
export function Transcript({
  entries,
  assistantSpeaking,
  placeholder,
}: TranscriptProps) {
  const scrollRef = useRef<ComponentRef<typeof ScrollView>>(null);
  const lastAssistantId = [...entries]
    .reverse()
    .find((entry) => entry.role === 'assistant')?.id;

  return (
    <View style={styles.container}>
      <ScrollView
        ref={scrollRef}
        contentContainerStyle={styles.content}
        onContentSizeChange={() =>
          scrollRef.current?.scrollToEnd({ animated: true })
        }
      >
        {entries.length === 0 ? (
          <Text style={styles.placeholder}>{placeholder}</Text>
        ) : (
          entries.map((entry) =>
            entry.role === 'user' ? (
              <View key={entry.id} style={styles.userBubble}>
                <Text style={styles.userText}>{entry.text}</Text>
              </View>
            ) : (
              <Text
                key={entry.id}
                style={[
                  styles.assistantText,
                  assistantSpeaking &&
                    entry.id === lastAssistantId &&
                    styles.liveText,
                ]}
              >
                {entry.text}
                {entry.interrupted && (
                  <Text style={styles.interrupted}> · interrupted</Text>
                )}
              </Text>
            )
          )
        )}
      </ScrollView>
      <View style={styles.fade} pointerEvents="none" />
    </View>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    // Keeps older lines from scrolling up under the transparent header.
    overflow: 'hidden',
  },
  content: {
    flexGrow: 1,
    justifyContent: 'flex-end',
    gap: 10,
    paddingTop: fadeHeight,
    paddingBottom: 8,
  },
  fade: {
    position: 'absolute',
    top: 0,
    left: 0,
    right: 0,
    height: fadeHeight,
    experimental_backgroundImage: `linear-gradient(180deg, ${colors.background} 0%, rgba(35, 39, 54, 0) 100%)`,
  },
  placeholder: {
    alignSelf: 'center',
    maxWidth: 300,
    textAlign: 'center',
    fontSize: 15,
    lineHeight: 21,
    color: colors.textMuted,
  },
  assistantText: {
    paddingHorizontal: 6,
    fontSize: 15.5,
    lineHeight: 22,
    color: colors.textSecondary,
  },
  liveText: {
    color: colors.textPrimary,
  },
  interrupted: {
    fontSize: 13,
    color: colors.textMuted,
  },
  userBubble: {
    alignSelf: 'flex-end',
    maxWidth: '80%',
    paddingHorizontal: 13,
    paddingVertical: 9,
    borderRadius: 18,
    borderBottomRightRadius: 6,
    // The blue hue at 22% over the surface colour.
    backgroundColor: '#254e6e',
  },
  userText: {
    fontSize: 15.5,
    lineHeight: 21,
    color: colors.textPrimary,
  },
});
