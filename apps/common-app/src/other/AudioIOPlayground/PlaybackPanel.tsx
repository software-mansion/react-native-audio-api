import React, { FC, useState } from 'react';

import {
  ButtonRow,
  Call,
  CallButton,
  ChoiceSelect,
  Field,
  LabeledSwitch,
  Section,
  StatusRow,
  StatusText,
} from './ui';
import {
  PLAYBACK_CALLS,
  PLAYBACK_SOURCE_NAMES,
  PlaybackSourceName,
  usePlayback,
} from './usePlayback';

const SOURCE_SHORT_LABELS: Record<PlaybackSourceName, string> = {
  'Last take': 'Last take',
  'Music sample': 'Music',
  'Voice sample': 'Voice',
};

interface PlaybackPanelProps {
  playback: ReturnType<typeof usePlayback>;
  lastTakePath: string | null;
}

function fileName(path: string): string {
  return path.split('/').pop() ?? path;
}

const PlaybackPanel: FC<PlaybackPanelProps> = ({
  playback: { status, options, play, pause, resume, stop, disconnect },
  lastTakePath,
}) => {
  const [sourceName, setSourceName] =
    useState<PlaybackSourceName>('Music sample');
  const [loop, setLoop] = useState(false);

  return (
    <Section title="Playback" shortTitle="Play">
      <StatusRow>
        <StatusText testID="playback-status" text={status} />
        <Call text={PLAYBACK_CALLS.status} />
        <StatusText
          testID="playback-last-take"
          text={`last take: ${lastTakePath ? fileName(lastTakePath) : 'none'}`}
        />
        <StatusText testID="playback-options" text={JSON.stringify(options)} />
        <Call text={PLAYBACK_CALLS.options} />
      </StatusRow>
      <Field label="Source" shortLabel="Src">
        <ButtonRow>
          <ChoiceSelect
            testID="playback-source"
            value={sourceName}
            options={PLAYBACK_SOURCE_NAMES}
            shortLabels={SOURCE_SHORT_LABELS}
            onChange={setSourceName}
          />
          <LabeledSwitch
            testID="playback-loop"
            label="Loop"
            shortLabel="Loop"
            value={loop}
            onValueChange={setLoop}
          />
        </ButtonRow>
      </Field>
      <ButtonRow>
        <CallButton
          testID="playback-play"
          title="Play"
          shortTitle="▶ Play"
          call={PLAYBACK_CALLS.play}
          onPress={() => play(sourceName, loop)}
        />
        <CallButton
          testID="playback-pause"
          title="Pause"
          shortTitle="⏸ Pause"
          call={PLAYBACK_CALLS.pause}
          onPress={pause}
        />
        <CallButton
          testID="playback-resume"
          title="Resume"
          shortTitle="▶ Resume"
          call={PLAYBACK_CALLS.resume}
          onPress={resume}
        />
        <CallButton
          testID="playback-stop"
          title="Stop"
          shortTitle="■ Stop"
          call={PLAYBACK_CALLS.stop}
          onPress={stop}
        />
        <CallButton
          testID="playback-disconnect"
          title="Disconnect"
          shortTitle="Disconnect"
          call={PLAYBACK_CALLS.disconnect}
          onPress={disconnect}
        />
      </ButtonRow>
    </Section>
  );
};

export default PlaybackPanel;
