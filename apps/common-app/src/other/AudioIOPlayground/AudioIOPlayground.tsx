import React, { FC, useState } from 'react';
import { ScrollView, StyleSheet } from 'react-native';

import { Container } from '../../components';
import { layout } from '../../styles';
import DevicePanel from './DevicePanel';
import { useLog } from './log';
import LogView from './LogView';
import PlaybackPanel from './PlaybackPanel';
import RecorderPanel from './RecorderPanel';
import SystemPanel from './SystemPanel';
import { CompactContext } from './ui';
import { usePlayback } from './usePlayback';
import { useRecorder } from './useRecorder';
import { useSystemEventLog } from './useSystemEventLog';

const AudioIOPlayground: FC = () => {
  const [isCompact, setCompact] = useState(false);
  const { entries, log, clear } = useLog();
  const [lastTakePath, setLastTakePath] = useState<string | null>(null);
  useSystemEventLog(log);
  // Owned here, not in their panels, because the System panel recreates them.
  const recorder = useRecorder(log, setLastTakePath);
  const playback = usePlayback(log, lastTakePath);

  return (
    <CompactContext.Provider value={isCompact}>
      <Container disablePadding>
        <ScrollView
          contentContainerStyle={[
            styles.panels,
            isCompact && styles.compactPanels,
          ]}
        >
          <SystemPanel
            log={log}
            onRecreateRecorder={recorder.recreate}
            onRecreateContext={playback.recreate}
          />
          <RecorderPanel recorder={recorder} />
          <PlaybackPanel playback={playback} lastTakePath={lastTakePath} />
          <DevicePanel log={log} />
        </ScrollView>
        <LogView
          entries={entries}
          onToggleCompact={() => setCompact((current) => !current)}
          onClear={clear}
        />
      </Container>
    </CompactContext.Provider>
  );
};

export default AudioIOPlayground;

const styles = StyleSheet.create({
  panels: {
    paddingHorizontal: 2 * layout.spacing,
  },
  compactPanels: {
    paddingHorizontal: layout.spacing,
  },
});
