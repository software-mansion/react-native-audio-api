import React, { FC, useState } from 'react';

import { Select } from '../../components';
import type { Log } from './log';
import {
  Call,
  CallButton,
  Field,
  LabeledSwitch,
  Section,
  StatusText,
} from './ui';
import {
  PLAYBACK_CALLS,
  PLAYBACK_SOURCE_NAMES,
  PlaybackSourceName,
  usePlayback,
} from './usePlayback';

interface PlaybackPanelProps {
  log: Log;
  lastTakePath: string | null;
}

function fileName(path: string): string {
  return path.split('/').pop() ?? path;
}

const PlaybackPanel: FC<PlaybackPanelProps> = ({ log, lastTakePath }) => {
  const [sourceName, setSourceName] =
    useState<PlaybackSourceName>('Music sample');
  const [loop, setLoop] = useState(false);
  const { status, play, pause, resume, stop } = usePlayback(log, lastTakePath);

  return (
    <Section title="Playback">
      <StatusText testID="playback-status" text={status} />
      <Call text={PLAYBACK_CALLS.status} />
      <StatusText
        testID="playback-last-take"
        text={`last take: ${lastTakePath ? fileName(lastTakePath) : 'none'}`}
      />
      <Field label="Source">
        <Select
          testID="playback-source"
          value={sourceName}
          options={PLAYBACK_SOURCE_NAMES}
          onChange={setSourceName}
        />
      </Field>
      <LabeledSwitch
        testID="playback-loop"
        label="Loop"
        value={loop}
        onValueChange={setLoop}
      />
      <CallButton
        testID="playback-play"
        title="Play"
        call={PLAYBACK_CALLS.play}
        onPress={() => play(sourceName, loop)}
      />
      <CallButton
        testID="playback-pause"
        title="Pause"
        call={PLAYBACK_CALLS.pause}
        onPress={pause}
      />
      <CallButton
        testID="playback-resume"
        title="Resume"
        call={PLAYBACK_CALLS.resume}
        onPress={resume}
      />
      <CallButton
        testID="playback-stop"
        title="Stop"
        call={PLAYBACK_CALLS.stop}
        onPress={stop}
      />
    </Section>
  );
};

export default PlaybackPanel;
