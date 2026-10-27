import React, { FC, useState } from 'react';
import { ScrollView, StyleSheet } from 'react-native';

import { Container } from '../../components';
import { layout } from '../../styles';
import DevicePanel from './DevicePanel';
import { useLog } from './log';
import LogView from './LogView';
import PlaybackPanel from './PlaybackPanel';
import RecorderPanel from './RecorderPanel';
import SessionPanel from './SessionPanel';
import { useSystemEventLog } from './useSystemEventLog';

const AudioIOPlayground: FC = () => {
  const { entries, log, clear } = useLog();
  const [lastTakePath, setLastTakePath] = useState<string | null>(null);
  useSystemEventLog(log);

  return (
    <Container disablePadding>
      <ScrollView contentContainerStyle={styles.panels}>
        <SessionPanel log={log} />
        <RecorderPanel log={log} onTake={setLastTakePath} />
        <PlaybackPanel log={log} lastTakePath={lastTakePath} />
        <DevicePanel log={log} />
      </ScrollView>
      <LogView entries={entries} onClear={clear} />
    </Container>
  );
};

export default AudioIOPlayground;

const styles = StyleSheet.create({
  panels: {
    paddingHorizontal: 2 * layout.spacing,
  },
});
