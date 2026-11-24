import { useNavigation } from '@react-navigation/native';
import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import {
  BackHandler,
  Image,
  Pressable,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {
  AudioBuffer,
  AudioManager,
  PlaybackNotificationManager,
} from 'react-native-audio-api';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { AppIcon, GlassSurface, ListSection } from '../../components';
import { colors, layout } from '../../styles';
import AudioPlayer from '../../utils/AudioPlayer';
import { audioContext } from '../../singletons';
import { activatePlaybackSession } from '../../utils/audio';
import { TRACKS, type Track } from '../Crossfade/tracks';
import { createMoodChain, type Mood } from './moodFilters';
import { createPlaybackPosition } from './playbackPosition';
import PlayerSheet, { miniPlayerHeight, SheetState } from './PlayerSheet';

/** The demo's six tracks twice, for a longer queue. */
const playlist = [...TRACKS, ...TRACKS];

/**
 * A track that starts within this long (ms) shows no spinner: a preloaded one
 * starts in a few frames, and a spinner shown for those flashed on every
 * swipe.
 */
const spinnerDelayMs = 250;

/**
 * Playlist steps around the current track kept decoded: the queue's left and
 * right neighbours, so a swipe or Previous / Next either way starts at once.
 */
const preloadOffsets = [-1, 0, 1];

const rowInset = 12;
const coverSize = 48;
const backButtonSize = 44;

const dbToGain = (db: number) => 10 ** (db / 20);

async function showTrackNotification(track: Track, duration: number) {
  await PlaybackNotificationManager.show({
    title: track.title,
    artist: track.artist,
    album: 'Audio API',
    androidSmallIcon: 'logo',
    duration,
    state: 'playing',
    speed: 1,
    elapsedTime: 0,
  });
}

interface TrackRowProps {
  track: Track;
  isCurrent: boolean;
  onPress: () => void;
}

const TrackRow: FC<TrackRowProps> = ({ track, isCurrent, onPress }) => (
  <Pressable
    onPress={onPress}
    accessibilityRole="button"
    accessibilityLabel={`${track.title}, ${track.artist}`}
    accessibilityState={{ selected: isCurrent }}
    style={({ pressed }) => [styles.row, pressed && styles.rowPressed]}
  >
    <Image source={track.cover} style={styles.rowCover} />
    <View style={styles.rowTitles}>
      <Text
        style={[styles.rowTitle, isCurrent && styles.rowTitleCurrent]}
        numberOfLines={1}
      >
        {track.title}
      </Text>
      <Text style={styles.rowArtist} numberOfLines={1}>
        {track.artist}
      </Text>
    </View>
    {isCurrent && (
      <AppIcon name="audio-lines" size={20} color={colors.accent} />
    )}
  </Pressable>
);

/**
 * A music player: pick a track from the playlist and it plays in a mini player
 * docked at the bottom, which expands into a full player with the queue,
 * progress and transport controls.
 */
const Player: FC = () => {
  const [currentIndex, setCurrentIndex] = useState<number | null>(null);
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [duration, setDuration] = useState(0);
  const [sheetState, setSheetState] = useState(SheetState.Mini);

  const insets = useSafeAreaInsets();
  const navigation = useNavigation();
  const onTrackEndedRef = useRef(() => {});
  const [position] = useState(createPlaybackPosition);
  // The app-wide context lives as long as the app, so the player can too.
  const [moodChain] = useState(() => createMoodChain(audioContext));
  const [player] = useState(() => {
    const created = new AudioPlayer(audioContext, {
      onPositionChanged: position.set,
      onEnded: () => onTrackEndedRef.current(),
    });
    created.setOutput(moodChain.input);
    return created;
  });
  const [mood, setMood] = useState<Mood>('off');

  const changeMood = useCallback(
    (next: Mood) => {
      moodChain.apply(next);
      setMood(next);
    },
    [moodChain]
  );

  /**
   * Decodes by playlist position, shared by playback and preloading. Holds the
   * current track and its neighbours (`preloadOffsets`); the rest are dropped
   * to bound memory. A failed decode is forgotten, so it is retried next time.
   */
  const decodes = useRef(new Map<number, Promise<AudioBuffer>>());
  const decode = useCallback((index: number) => {
    let decoding = decodes.current.get(index);
    if (!decoding) {
      decoding = audioContext.decodeAudioData(playlist[index].audio);
      decoding.catch(() => decodes.current.delete(index));
      decodes.current.set(index, decoding);
    }
    return decoding;
  }, []);

  const preloadAround = useCallback(
    (index: number) => {
      // Wraps around like Next and the queue's ends.
      const keep = preloadOffsets.map(
        (offset) => (index + offset + playlist.length) % playlist.length
      );
      for (const cached of decodes.current.keys()) {
        if (!keep.includes(cached)) {
          decodes.current.delete(cached);
        }
      }
      // Failures surface when the track is played.
      keep.forEach((ahead) => decode(ahead).catch(() => {}));
    },
    [decode]
  );

  const spinnerTimer = useRef<ReturnType<typeof setTimeout>>(undefined);

  const playTrack = useCallback(
    async (index: number) => {
      const track = playlist[index];
      setCurrentIndex(index);
      setIsPlaying(true);
      // A newer pick takes the timer over, so an overtaken one can't show it.
      clearTimeout(spinnerTimer.current);
      spinnerTimer.current = setTimeout(
        () => setIsLoading(true),
        spinnerDelayMs
      );
      const stopSpinner = () => {
        clearTimeout(spinnerTimer.current);
        setIsLoading(false);
      };

      // Before any await: a quick second swipe then finds its track already
      // loading as this one's neighbour instead of starting the download.
      preloadAround(index);
      const decoded = decode(index);

      try {
        await activatePlaybackSession();
        if (!(await player.load(decoded))) {
          return; // Another track was picked while this one decoded.
        }
        player.setVolume(dbToGain(track.levelDb));
        setDuration(player.getDuration());
        await player.play();
        stopSpinner();
        await showTrackNotification(track, player.getDuration());
      } catch (error) {
        console.error('Failed to play track:', error);
        setIsPlaying(false);
        stopSpinner();
      }
    },
    [decode, player, preloadAround]
  );

  const togglePlayback = useCallback(async () => {
    if (isPlaying) {
      await player.pause();
      setIsPlaying(false);
    } else {
      await activatePlaybackSession();
      await player.play();
      setIsPlaying(true);
    }
  }, [isPlaying, player]);

  const playNext = useCallback(() => {
    if (currentIndex !== null) {
      playTrack((currentIndex + 1) % playlist.length);
    }
  }, [currentIndex, playTrack]);

  const playPrevious = useCallback(() => {
    if (currentIndex === null) {
      return;
    }
    if (currentIndex === 0) {
      player.seekBy(-player.getElapsedTime());
      return;
    }
    playTrack(currentIndex - 1);
  }, [currentIndex, playTrack, player]);

  const seek = useCallback(
    (seconds: number) => {
      player.seekTo(seconds);
      // Shown at once; the player reports positions only while playing.
      position.set(duration > 0 ? seconds / duration : 0);
    },
    [duration, player, position]
  );

  // A finished track moves on to the next one, like a playlist.
  useEffect(() => {
    onTrackEndedRef.current = playNext;
  }, [playNext]);

  useEffect(
    () => () => {
      clearTimeout(spinnerTimer.current);
      player.dispose();
      // The chain lives on the app-wide context, so it's unplugged here.
      moodChain.disconnect();
      PlaybackNotificationManager.hide();
      AudioManager.setAudioSessionActivity(false);
    },
    [moodChain, player]
  );

  useEffect(() => {
    AudioManager.requestNotificationPermissions();
    PlaybackNotificationManager.enableControl('play', true);
    PlaybackNotificationManager.enableControl('pause', true);
    PlaybackNotificationManager.enableControl('nextTrack', true);
    PlaybackNotificationManager.enableControl('previousTrack', true);
    PlaybackNotificationManager.enableControl('seekTo', true);
  }, []);

  useEffect(() => {
    const subscriptions = [
      PlaybackNotificationManager.addEventListener(
        'playbackNotificationPlay',
        () => !isPlaying && togglePlayback()
      ),
      PlaybackNotificationManager.addEventListener(
        'playbackNotificationPause',
        () => isPlaying && togglePlayback()
      ),
      PlaybackNotificationManager.addEventListener(
        'playbackNotificationNextTrack',
        playNext
      ),
      PlaybackNotificationManager.addEventListener(
        'playbackNotificationPreviousTrack',
        playPrevious
      ),
      PlaybackNotificationManager.addEventListener(
        'playbackNotificationSeekTo',
        (event) => seek(event.value)
      ),
    ];
    return () => subscriptions.forEach((subscription) => subscription.remove());
  }, [isPlaying, togglePlayback, playNext, playPrevious, seek]);

  // The full player behaves like a full-screen view: it closes by dragging it
  // down or its chevron, never by a back swipe (iOS 26's content-wide one would
  // also fire while swiping the queue).
  useEffect(() => {
    const isFull = sheetState === SheetState.Full;
    navigation.setOptions({
      gestureEnabled: !isFull,
      fullScreenGestureEnabled: !isFull,
    });
  }, [navigation, sheetState]);

  // Android's back button collapses the full player before leaving the screen.
  useEffect(() => {
    if (sheetState !== SheetState.Full) {
      return;
    }
    const subscription = BackHandler.addEventListener(
      'hardwareBackPress',
      () => {
        setSheetState(SheetState.Mini);
        return true;
      }
    );
    return () => subscription.remove();
  }, [sheetState]);

  const isSheetShown = currentIndex !== null;

  return (
    <View style={styles.screen}>
      <ScrollView
        contentContainerStyle={[
          styles.list,
          { paddingTop: insets.top + backButtonSize + layout.spacing * 2 },
          isSheetShown && {
            paddingBottom: insets.bottom + miniPlayerHeight * 2,
          },
        ]}
      >
        <ListSection
          title={`Playlist · ${playlist.length} tracks`}
          separatorInset={rowInset + coverSize + rowInset}
        >
          {playlist.map((track, index) => (
            <TrackRow
              key={index}
              track={track}
              isCurrent={index === currentIndex}
              onPress={() => playTrack(index)}
            />
          ))}
        </ListSection>
      </ScrollView>
      <Pressable
        onPress={navigation.goBack}
        accessibilityRole="button"
        accessibilityLabel="Back"
        style={[styles.backButton, { top: insets.top + layout.spacing }]}
      >
        {({ pressed }) => (
          <GlassSurface interactive pressed={pressed} style={styles.backGlass}>
            <AppIcon
              name="chevron-right"
              size={22}
              color={colors.textPrimary}
              style={styles.backIcon}
            />
          </GlassSurface>
        )}
      </Pressable>
      {currentIndex !== null && (
        <PlayerSheet
          tracks={playlist}
          currentIndex={currentIndex}
          isPlaying={isPlaying}
          isLoading={isLoading}
          position={position}
          duration={duration}
          state={sheetState}
          onStateChange={setSheetState}
          onTogglePlayback={togglePlayback}
          onNext={playNext}
          onPrevious={playPrevious}
          onSelectTrack={playTrack}
          onSeek={seek}
          mood={mood}
          onMoodChange={changeMood}
        />
      )}
    </View>
  );
};

export default Player;

const styles = StyleSheet.create({
  screen: {
    flex: 1,
    backgroundColor: colors.background,
  },
  list: {
    padding: layout.screenPadding,
  },
  backButton: {
    position: 'absolute',
    left: layout.screenPadding,
  },
  backGlass: {
    width: backButtonSize,
    height: backButtonSize,
    borderRadius: backButtonSize / 2,
    alignItems: 'center',
    justifyContent: 'center',
  },
  backIcon: {
    transform: [{ rotate: '180deg' }],
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: rowInset,
    padding: rowInset,
  },
  rowPressed: {
    backgroundColor: colors.surfacePressed,
  },
  rowCover: {
    width: coverSize,
    height: coverSize,
    borderRadius: 8,
  },
  rowTitles: {
    flex: 1,
    gap: 2,
  },
  rowTitle: {
    color: colors.textPrimary,
    fontSize: 16,
    fontWeight: '500',
  },
  rowTitleCurrent: {
    color: colors.accent,
  },
  rowArtist: {
    color: colors.textMuted,
    fontSize: 13,
  },
});
