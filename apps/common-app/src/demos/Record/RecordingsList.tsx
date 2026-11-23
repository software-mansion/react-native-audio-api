import React, { useCallback, useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import { AudioBufferSourceNode, AudioManager } from 'react-native-audio-api';

import { ListSection, RoundButton } from '../../components';
import { audioContext } from '../../singletons';
import { colors } from '../../styles';
import { formatClock } from '../../utils/formatClock';
import { Recording, useRecordings } from './recordingsStore';

const rowPadding = 14;
const playButtonSize = 32;
const previewHeight = 22;

function dayLabel(date: Date) {
  const startOfDay = (day: Date) =>
    new Date(day.getFullYear(), day.getMonth(), day.getDate()).getTime();
  const daysAgo = Math.round(
    (startOfDay(new Date()) - startOfDay(date)) / 86_400_000
  );
  if (daysAgo === 0) {
    return 'Today';
  }
  if (daysAgo === 1) {
    return 'Yesterday';
  }
  return date.toLocaleDateString(undefined, { month: 'short', day: 'numeric' });
}

const Preview: React.FC<{ levels: number[] }> = ({ levels }) => (
  <View style={styles.preview} accessible={false}>
    {levels.map((level, index) => (
      <View
        key={index}
        style={[
          styles.previewBar,
          { height: Math.max(2.2, level * previewHeight) },
        ]}
      />
    ))}
  </View>
);

interface RecordingRowProps {
  recording: Recording;
  isPlaying: boolean;
  onTogglePlay: (recording: Recording) => void;
}

const RecordingRow: React.FC<RecordingRowProps> = ({
  recording,
  isPlaying,
  onTogglePlay,
}) => (
  <View style={styles.row}>
    <View style={styles.titles}>
      <Text style={styles.title} numberOfLines={1}>
        {recording.title}
      </Text>
      <Text style={styles.subtitle}>
        {dayLabel(recording.createdAt)} · {formatClock(recording.duration)}
      </Text>
    </View>
    <Preview levels={recording.preview} />
    <RoundButton
      variant="glass"
      icon={isPlaying ? 'pause' : 'play'}
      accessibilityLabel={`${isPlaying ? 'Stop' : 'Play'} ${recording.title}`}
      onPress={() => onTogglePlay(recording)}
      size={playButtonSize}
      iconSize={15}
    />
  </View>
);

interface RecordingsListProps {
  /** Stops playback and blocks it, e.g. while a new take is recorded. */
  disabled: boolean;
}

/** The takes recorded this session, newest first, each playable in place. */
const RecordingsList: React.FC<RecordingsListProps> = ({ disabled }) => {
  const recordings = useRecordings();
  const [playingId, setPlayingId] = useState<number | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  // Bumped by every stop, so a play still awaiting the session sees it lost.
  const playGenerationRef = useRef(0);

  const stop = useCallback(() => {
    playGenerationRef.current += 1;
    try {
      // A source that already ended may throw; there is nothing left to stop.
      sourceRef.current?.stop();
    } catch {}
    sourceRef.current?.disconnect();
    sourceRef.current = null;
    setPlayingId(null);
  }, []);

  const onTogglePlay = useCallback(
    async (recording: Recording) => {
      const wasPlaying = playingId === recording.id;
      stop();
      if (wasPlaying || disabled) {
        return;
      }

      const generation = playGenerationRef.current;
      await AudioManager.setAudioSessionActivity(true);
      if (generation !== playGenerationRef.current) {
        return;
      }

      const source = audioContext.createBufferSource();
      source.buffer = recording.buffer;
      source.connect(audioContext.destination);
      source.onended = () => {
        if (sourceRef.current === source) {
          stop();
        }
      };
      source.start(audioContext.currentTime);
      // The source must stay referenced until it ends, or it can be collected
      // mid-playback.
      sourceRef.current = source;
      setPlayingId(recording.id);
    },
    [disabled, playingId, stop]
  );

  useEffect(() => {
    if (disabled) {
      stop();
    }
  }, [disabled, stop]);

  useEffect(() => stop, [stop]);

  if (recordings.length === 0) {
    return (
      <View style={styles.empty}>
        <Text style={styles.emptyTitle}>No recordings</Text>
        <Text style={styles.emptyHint}>Takes you record appear here.</Text>
      </View>
    );
  }

  return (
    <ListSection title="Recordings" separatorInset={rowPadding}>
      {recordings.map((recording) => (
        <RecordingRow
          key={recording.id}
          recording={recording}
          isPlaying={playingId === recording.id}
          onTogglePlay={onTogglePlay}
        />
      ))}
    </ListSection>
  );
};

export default RecordingsList;

const styles = StyleSheet.create({
  empty: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    gap: 4,
  },
  emptyTitle: {
    fontSize: 17,
    fontWeight: '600',
    color: colors.textSecondary,
  },
  emptyHint: {
    fontSize: 15,
    color: colors.textMuted,
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
    paddingVertical: 10,
    paddingLeft: rowPadding,
    paddingRight: 12,
  },
  titles: {
    flex: 1,
  },
  title: {
    fontSize: 17,
    color: colors.textPrimary,
  },
  subtitle: {
    marginTop: 2,
    fontSize: 13,
    fontVariant: ['tabular-nums'],
    color: colors.textMuted,
  },
  preview: {
    height: previewHeight,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 1.2,
  },
  previewBar: {
    width: 2.2,
    borderRadius: 1.1,
    backgroundColor: colors.textMuted,
  },
});
