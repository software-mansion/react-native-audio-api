import React, { FC, useState } from 'react';
import { ScrollView, StyleSheet } from 'react-native';

import { Container } from '../../components';
import { layout } from '../../styles';
import DevicePanel from './DevicePanel';
import { logToConsole } from './log';
import PlaybackPanel from './PlaybackPanel';
import RecorderPanel from './RecorderPanel';
import SystemPanel from './SystemPanel';
import { usePlayback } from './usePlayback';
import { useRecorder } from './useRecorder';
import { useSystemEventLog } from './useSystemEventLog';

const AudioIOPlayground: FC = () => {
  const [lastTakePath, setLastTakePath] = useState<string | null>(null);
  useSystemEventLog(logToConsole);
  // Owned here, not in their panels, because the System panel recreates them.
  const recorder = useRecorder(logToConsole, setLastTakePath);
  const playback = usePlayback(logToConsole, lastTakePath);

  return (
    <Container disablePadding>
      <ScrollView contentContainerStyle={styles.panels}>
        <SystemPanel
          log={logToConsole}
          onRecreateRecorder={recorder.recreate}
          onRecreateContext={playback.recreate}
        />
        <RecorderPanel recorder={recorder} />
        <PlaybackPanel playback={playback} lastTakePath={lastTakePath} />
        <DevicePanel log={logToConsole} />
      </ScrollView>
    </Container>
  );
};

export default AudioIOPlayground;

const styles = StyleSheet.create({
  panels: {
    paddingHorizontal: 2 * layout.spacing,
  },
});
