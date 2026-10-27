import React, { FC } from 'react';

import type { Log } from './log';
import { Call, CallButton, Section, StatusText } from './ui';
import { RECORDER_CALLS, useRecorder } from './useRecorder';

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
      <Call text={RECORDER_CALLS.status} />
      <StatusText testID="recorder-level" text={level} />
      <Call text={RECORDER_CALLS.level} />
      <CallButton
        testID="recorder-start"
        title="Record"
        call={RECORDER_CALLS.start}
        onPress={start}
      />
      <CallButton
        testID="recorder-pause"
        title="Pause"
        call={RECORDER_CALLS.pause}
        onPress={pause}
      />
      <CallButton
        testID="recorder-resume"
        title="Resume"
        call={RECORDER_CALLS.resume}
        onPress={resume}
      />
      <CallButton
        testID="recorder-stop"
        title="Stop"
        call={RECORDER_CALLS.stop}
        onPress={stop}
      />
    </Section>
  );
};

export default RecorderPanel;
