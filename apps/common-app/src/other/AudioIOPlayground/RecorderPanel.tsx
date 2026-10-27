import React, { FC } from 'react';

import { Button } from '../../components';
import type { Log } from './log';
import { ButtonRow, Section, StatusText } from './ui';
import { useRecorder } from './useRecorder';

interface RecorderPanelProps {
  log: Log;
  onTake: (path: string) => void;
}

const RecorderPanel: FC<RecorderPanelProps> = ({ log, onTake }) => {
  const { status, level, start, pause, resume, stop } = useRecorder(
    log,
    onTake
  );

  return (
    <Section title="Recorder">
      <StatusText testID="recorder-status" text={status} />
      <StatusText testID="recorder-level" text={level} />
      <ButtonRow>
        <Button testID="recorder-start" title="Record" onPress={start} />
        <Button testID="recorder-pause" title="Pause" onPress={pause} />
        <Button testID="recorder-resume" title="Resume" onPress={resume} />
        <Button testID="recorder-stop" title="Stop" onPress={stop} />
      </ButtonRow>
    </Section>
  );
};

export default RecorderPanel;
