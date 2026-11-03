import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { Button, Container } from '../../components';
import { colors, layout } from '../../styles';
import { Transcript } from './components/Transcript';
import { useVoiceChat } from './hooks/useVoiceChat';
import type { SessionStatus } from './voiceSession';

const ERROR_COLOR = '#ff8d8d';

const statusColor: Record<SessionStatus, string> = {
  idle: colors.gray,
  connecting: colors.yellow,
  connected: '#7bd88f',
  error: ERROR_COLOR,
};

const statusText: Record<SessionStatus, string> = {
  idle: 'Ready to connect',
  connecting: 'Connecting…',
  connected: 'Listening',
  error: 'Connection failed',
};

const VoiceChat: FC = () => {
  const { status, detail, speaking, entries, toggleConnection } =
    useVoiceChat();

  return (
    <Container>
      <View style={styles.content}>
        <Text style={styles.description}>
          Talk naturally. The assistant can respond while you speak, and your
          voice interrupts unfinished replies.
        </Text>
        <View style={styles.statusRow}>
          <View
            style={[styles.statusDot, { backgroundColor: statusColor[status] }]}
          />
          <Text style={styles.status}>
            {speaking ? 'Speech detected' : statusText[status]}
          </Text>
        </View>
        {detail ? <Text style={styles.error}>{detail}</Text> : null}

        <Transcript entries={entries} />

        <View style={styles.actions}>
          <Button
            title={
              status === 'connected' || status === 'connecting'
                ? 'End call'
                : 'Connect'
            }
            onPress={toggleConnection}
            width={160}
          />
        </View>
      </View>
    </Container>
  );
};

const styles = StyleSheet.create({
  content: {
    flex: 1,
    gap: layout.spacing * 2,
  },
  description: {
    color: colors.gray,
    fontSize: 14,
    lineHeight: 20,
  },
  statusRow: {
    alignItems: 'center',
    flexDirection: 'row',
    gap: layout.spacing,
  },
  statusDot: {
    borderRadius: 6,
    height: 12,
    width: 12,
  },
  status: {
    color: colors.white,
    fontSize: 16,
    fontWeight: '600',
  },
  error: {
    color: ERROR_COLOR,
    fontSize: 13,
  },
  actions: {
    alignItems: 'center',
    paddingBottom: layout.spacing,
  },
});

export default VoiceChat;
