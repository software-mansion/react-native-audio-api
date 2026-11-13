import React, { FC } from 'react';
import { StyleSheet, Text, useWindowDimensions, View } from 'react-native';

import { Container } from '../../components';
import { colors } from '../../styles';
import { CallControls } from './components/CallControls';
import { CallTimer } from './components/CallTimer';
import { Transcript } from './components/Transcript';
import { useVoiceChat } from './hooks/useVoiceChat';
import type { OrbState } from './orb/orbPalettes';
import VoiceOrb from './orb/VoiceOrb';
import type { SessionStatus } from './voiceSession';

const ERROR_COLOR = '#ff8d8d';
const ORB_SHARE_OF_WIDTH = 0.58;
const MAX_ORB_SIZE = 240;

const IDLE_PLACEHOLDER =
  'Talk naturally. The assistant can respond while you speak, and your voice interrupts unfinished replies.';
const IN_CALL_PLACEHOLDER = 'The assistant is listening. Say hello to start.';

function orbState(
  status: SessionStatus,
  assistantSpeaking: boolean,
  muted: boolean
): OrbState {
  switch (status) {
    case 'connected':
      if (assistantSpeaking) {
        return 'speaking';
      }
      return muted ? 'paused' : 'listening';
    case 'error':
      return 'error';
    default:
      return 'idle';
  }
}

function statusWord(
  status: SessionStatus,
  assistantSpeaking: boolean,
  muted: boolean
): string | null {
  switch (status) {
    case 'connecting':
      return 'Connecting';
    case 'connected':
      if (assistantSpeaking) {
        return 'Speaking';
      }
      return muted ? 'Muted' : 'Listening';
    case 'error':
      return 'Couldn’t connect';
    default:
      return null;
  }
}

const VoiceChat: FC = () => {
  const { width } = useWindowDimensions();
  const orbSize = Math.min(
    MAX_ORB_SIZE,
    Math.round(width * ORB_SHARE_OF_WIDTH)
  );
  const {
    status,
    detail,
    speaking,
    assistantSpeaking,
    muted,
    connectedAt,
    entries,
    toggleConnection,
    toggleMuted,
  } = useVoiceChat();

  const inCall = status === 'connecting' || status === 'connected';
  const word = statusWord(status, assistantSpeaking, muted);
  const orb = orbState(status, assistantSpeaking, muted);
  const orbAction = inCall ? 'End conversation' : 'Start conversation';

  return (
    <Container>
      <View style={styles.content}>
        <Transcript
          entries={entries}
          assistantSpeaking={assistantSpeaking}
          placeholder={inCall ? IN_CALL_PLACEHOLDER : IDLE_PLACEHOLDER}
        />

        <View style={styles.orbSection}>
          <VoiceOrb
            state={orb}
            hearing={orb === 'listening' && speaking}
            size={orbSize}
            onPress={toggleConnection}
            accessibilityLabel={word ? `${orbAction}. ${word}` : orbAction}
          />
          <View style={styles.statusBlock} accessibilityLiveRegion="polite">
            {word && (
              <Text
                style={[styles.status, status === 'error' && styles.errorText]}
              >
                {word}
              </Text>
            )}
            {status === 'connected' && connectedAt !== null ? (
              <CallTimer since={connectedAt} style={styles.timer} />
            ) : detail ? (
              <Text style={styles.detail} numberOfLines={3}>
                {detail}
              </Text>
            ) : null}
          </View>
        </View>

        <CallControls
          inCall={inCall}
          muted={muted}
          onStart={toggleConnection}
          onEnd={toggleConnection}
          onToggleMuted={toggleMuted}
        />
      </View>
    </Container>
  );
};

export default VoiceChat;

const styles = StyleSheet.create({
  content: {
    flex: 1,
    gap: 20,
    paddingBottom: 16,
  },
  orbSection: {
    alignItems: 'center',
    gap: 14,
  },
  statusBlock: {
    minHeight: 46,
    alignItems: 'center',
    gap: 2,
  },
  status: {
    fontSize: 18,
    fontWeight: '600',
    color: colors.textPrimary,
  },
  errorText: {
    color: ERROR_COLOR,
  },
  timer: {
    fontSize: 13,
    fontVariant: ['tabular-nums'],
    color: colors.textMuted,
  },
  detail: {
    maxWidth: 300,
    textAlign: 'center',
    fontSize: 13,
    lineHeight: 18,
    color: colors.textMuted,
  },
});
