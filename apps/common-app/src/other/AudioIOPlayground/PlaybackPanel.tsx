import React, { FC, useState } from 'react';

import { Button, Select } from '../../components';
import type { Log } from './log';
import { ButtonRow, Field, LabeledSwitch, Section, StatusText } from './ui';
import {
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
      <ButtonRow>
        <Button
          testID="playback-play"
          title="Play"
          onPress={() => play(sourceName, loop)}
        />
        <Button testID="playback-pause" title="Pause" onPress={pause} />
        <Button testID="playback-resume" title="Resume" onPress={resume} />
        <Button testID="playback-stop" title="Stop" onPress={stop} />
      </ButtonRow>
    </Section>
  );
};

export default PlaybackPanel;
