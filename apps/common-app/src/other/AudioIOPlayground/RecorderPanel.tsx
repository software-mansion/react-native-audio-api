import React, { FC } from 'react';

import {
  ButtonRow,
  Call,
  CallButton,
  Section,
  StatusRow,
  StatusText,
} from './ui';
import { RECORDER_CALLS, useRecorder } from './useRecorder';

interface RecorderPanelProps {
  recorder: ReturnType<typeof useRecorder>;
}

const RecorderPanel: FC<RecorderPanelProps> = ({
  recorder: { status, level, options, start, pause, resume, stop },
}) => (
  <Section title="Recorder" shortTitle="Rec">
    <StatusRow>
      <StatusText testID="recorder-status" text={status} />
      <Call text={RECORDER_CALLS.status} />
      <StatusText testID="recorder-level" text={level} />
      <Call text={RECORDER_CALLS.level} />
      <StatusText testID="recorder-options" text={JSON.stringify(options)} />
      <Call text={RECORDER_CALLS.options} />
    </StatusRow>
    <ButtonRow>
      <CallButton
        testID="recorder-start"
        title="Record"
        shortTitle="● Rec"
        call={RECORDER_CALLS.start}
        onPress={start}
      />
      <CallButton
        testID="recorder-pause"
        title="Pause"
        shortTitle="⏸ Pause"
        call={RECORDER_CALLS.pause}
        onPress={pause}
      />
      <CallButton
        testID="recorder-resume"
        title="Resume"
        shortTitle="▶ Resume"
        call={RECORDER_CALLS.resume}
        onPress={resume}
      />
      <CallButton
        testID="recorder-stop"
        title="Stop"
        shortTitle="■ Stop"
        call={RECORDER_CALLS.stop}
        onPress={stop}
      />
    </ButtonRow>
  </Section>
);

export default RecorderPanel;
