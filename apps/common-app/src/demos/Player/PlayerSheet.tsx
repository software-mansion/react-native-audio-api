import {
  BottomSheet,
  type Detent,
  type PositionChangeEventData,
} from '@swmansion/react-native-bottom-sheet';
import React, { FC, memo, useMemo, useState } from 'react';
import {
  ActivityIndicator,
  ImageSourcePropType,
  LayoutChangeEvent,
  LayoutRectangle,
  NativeSyntheticEvent,
  Pressable,
  StyleSheet,
  Text,
  useWindowDimensions,
  View,
} from 'react-native';
import Animated, {
  type FrameInfo,
  SharedValue,
  useAnimatedStyle,
  useEvent,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';
import { MenuView } from '@react-native-menu/menu';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import {
  AppIcon,
  GlassSurface,
  ProgressBar,
  RoundButton,
} from '../../components';
import { colors, layout, typography } from '../../styles';
import { formatClock } from '../../utils/formatClock';
import type { Track } from '../Crossfade/tracks';
import CoverBackdrop from '../Crossfade/CoverBackdrop';
import { type PlaybackPosition, usePlaybackPosition } from './playbackPosition';
import { type Mood, moodOrder, moods } from './moodFilters';
import QueueCarousel, { queueCoverSize } from './QueueCarousel';

/** Detent indices of the player sheet. */
export enum SheetState {
  Mini,
  Full,
}

export const miniPlayerHeight = 64;
const miniCoverSize = 44;
/**
 * How quickly the cover catches up with the sheet: each frame it closes the gap
 * as if decaying with this time constant (s). A follow spring restarted on
 * every position event either bounced (keeping its speed when the sheet stopped
 * short) or, with its overshoot clamped, snapped onto the raw position and lost
 * the smoothing. Exponential smoothing glides and can't overshoot.
 */
const coverFollowTime = 0.06;
/** Close enough to the sheet's position to land on it exactly. */
const coverFollowSnap = 0.001;
const miniPaddingHorizontal = 10;
/** Centre of the mini player's cover, in the sheet content's coordinates. */
const miniCoverCentre = {
  x: miniPaddingHorizontal + miniCoverSize / 2,
  y: miniPlayerHeight / 2,
};
const sideGap = 8;

interface PlayerSheetProps {
  tracks: Track[];
  currentIndex: number;
  isPlaying: boolean;
  isLoading: boolean;
  /** Played share of the current track; subscribed to by the progress bars only. */
  position: PlaybackPosition;
  /** Seconds. */
  duration: number;
  state: SheetState;
  onStateChange: (state: SheetState) => void;
  onTogglePlayback: () => void;
  onNext: () => void;
  onPrevious: () => void;
  onSelectTrack: (index: number) => void;
  /** Seconds into the current track. */
  onSeek: (seconds: number) => void;
  mood: Mood;
  onMoodChange: (mood: Mood) => void;
}

const clamp01 = (value: number) => {
  'worklet';
  return Math.min(1, Math.max(0, value));
};

interface TrackProgressProps {
  position: PlaybackPosition;
  /** Seconds. */
  duration: number;
  onSeek: (seconds: number) => void;
}

/** The mini player's thin progress line; the only part that ticks. */
const MiniProgress: FC<{ position: PlaybackPosition }> = ({ position }) => (
  <ProgressBar
    progress={usePlaybackPosition(position)}
    color={colors.textPrimary}
    height={2}
    style={styles.miniProgress}
  />
);

const moodActions = moodOrder.map((id) => ({
  id,
  title: moods[id].label,
  subtitle: moods[id].description,
}));

/**
 * Picks a mood filter for the track from the platform pull-down menu. The icon
 * takes the accent colour while a mood is on.
 */
const MoodButton: FC<{ mood: Mood; onChange: (mood: Mood) => void }> = ({
  mood,
  onChange,
}) => (
  <MenuView
    title="Mood"
    actions={moodActions.map((action) => ({
      ...action,
      state: action.id === mood ? ('on' as const) : ('off' as const),
    }))}
    onPressAction={({ nativeEvent }) => {
      const picked = moodOrder.find((id) => id === nativeEvent.event);
      if (picked) {
        onChange(picked);
      }
    }}
  >
    <View
      style={styles.moodButton}
      accessible
      accessibilityRole="button"
      accessibilityLabel={`Mood: ${moods[mood].label}`}
      accessibilityHint="Opens the mood filters"
    >
      <AppIcon
        name="audio-waveform"
        size={24}
        color={mood === 'off' ? colors.textPrimary : colors.accent}
      />
    </View>
  </MenuView>
);

/**
 * The full player's seekable progress and its times, which follow the finger
 * while seeking. Its own component, so a drag re-renders only this.
 */
const TrackProgress: FC<TrackProgressProps> = ({
  position,
  duration,
  onSeek,
}) => {
  const progress = usePlaybackPosition(position);
  const [scrubbed, setScrubbed] = useState<number | null>(null);
  const elapsed = (scrubbed ?? progress) * duration;

  return (
    <View>
      <ProgressBar
        progress={progress}
        color={colors.textPrimary}
        onScrub={setScrubbed}
        onSeek={(share) => onSeek(share * duration)}
      />
      <View style={styles.times}>
        <Text style={styles.time}>{formatClock(elapsed)}</Text>
        <Text style={styles.time}>−{formatClock(duration - elapsed)}</Text>
      </View>
    </View>
  );
};

/**
 * Glass under the mini player; while the sheet expands, the blurred covers fade
 * in over it as the full player's backdrop. Every cover is blurred once up
 * front and they blend with the queue's position, so changing track never blurs
 * a new full-screen image mid-animation.
 */
const PlayerSurface = memo<{
  covers: ImageSourcePropType[];
  queuePosition: SharedValue<number>;
  expansion: SharedValue<number>;
}>(({ covers, queuePosition, expansion }) => {
  const backdropStyle = useAnimatedStyle(() => ({
    opacity: expansion.value,
  }));

  return (
    <View style={StyleSheet.absoluteFill}>
      <GlassSurface style={StyleSheet.absoluteFill} />
      <Animated.View style={[StyleSheet.absoluteFill, backdropStyle]}>
        <CoverBackdrop covers={covers} progress={queuePosition} />
      </Animated.View>
    </View>
  );
});

/**
 * The player docked at the bottom of the screen. It floats as a mini player
 * showing the current track and expands, by dragging or tapping, into the full
 * player: the queue centred on the current track, progress and transport. The
 * library interpolates the detents' insets, so the floating card grows into the
 * full-width sheet as it moves.
 */
const PlayerSheet = memo<PlayerSheetProps>(function PlayerSheet({
  tracks,
  currentIndex,
  isPlaying,
  isLoading,
  position,
  duration,
  state,
  onStateChange,
  onTogglePlayback,
  onNext,
  onPrevious,
  onSelectTrack,
  onSeek,
  mood,
  onMoodChange,
}) {
  const insets = useSafeAreaInsets();
  const { width: screenWidth } = useWindowDimensions();
  const track = tracks[currentIndex];
  /** Where the queue sits in the full player, known once laid out. */
  const [queueFrame, setQueueFrame] = useState<LayoutRectangle | null>(null);
  const onQueueAreaLayout = (event: LayoutChangeEvent) =>
    setQueueFrame(event.nativeEvent.layout);
  const covers = useMemo(() => tracks.map(({ cover }) => cover), [tracks]);
  /** Fractional index of the centred cover; the queue moves it, by hand too. */
  const queuePosition = useSharedValue(currentIndex);

  /** 0 at the mini player, 1 at the full player, fractional while dragging. */
  const expansion = useSharedValue(state);
  const onPositionChange = useEvent<
    NativeSyntheticEvent<PositionChangeEventData>
  >(
    // Reanimated hands the worklet the unwrapped payload.
    (event: PositionChangeEventData) => {
      'worklet';
      expansion.value = clamp01(event.index);
    },
    ['onPositionChange']
  );

  const detents = useMemo<Detent[]>(
    () => [
      {
        value: miniPlayerHeight,
        insets: {
          left: sideGap,
          right: sideGap,
          bottom: insets.bottom + sideGap,
        },
      },
      '100%',
    ],
    [insets.bottom]
  );

  const miniStyle = useAnimatedStyle(() => ({
    opacity: 1 - clamp01(expansion.value * 3),
  }));
  const fullStyle = useAnimatedStyle(() => ({
    opacity: clamp01((expansion.value - 0.4) / 0.6),
  }));

  /**
   * The queue travels as one piece: at the mini player it is scaled down so its
   * current cover is the mini cover, and it grows into place as the sheet
   * opens, its neighbours fading in on the way. It is the same cover all the
   * way, so arriving has nothing to hand over.
   */
  const miniScale = miniCoverSize / queueCoverSize(screenWidth);
  /**
   * `expansion` smoothed for the cover. The sheet moves its card natively and
   * reports the position in events that don't arrive in step with the frames
   * the card moves on; scaled straight from them, the cover shook against the
   * card. Smoothing them per frame turns the uneven steps into a glide, and it
   * lands exactly on 0 or 1 at rest.
   */
  const coverProgress = useSharedValue(state as number);
  useFrameCallback(({ timeSincePreviousFrame }: FrameInfo) => {
    const target = expansion.value;
    const gap = target - coverProgress.value;
    if (gap === 0) {
      return;
    }
    const dt = (timeSincePreviousFrame ?? 16) / 1000;
    coverProgress.value =
      Math.abs(gap) < coverFollowSnap
        ? target
        : coverProgress.value + gap * (1 - Math.exp(-dt / coverFollowTime));
  });

  const queueStyle = useAnimatedStyle(() => {
    if (!queueFrame) {
      return {};
    }
    const opened = coverProgress.value;
    const centreX = queueFrame.x + queueFrame.width / 2;
    const centreY = queueFrame.y + queueFrame.height / 2;
    return {
      transform: [
        { translateX: (miniCoverCentre.x - centreX) * (1 - opened) },
        { translateY: (miniCoverCentre.y - centreY) * (1 - opened) },
        { scale: miniScale + (1 - miniScale) * opened },
      ],
    };
  });

  const isFull = state === SheetState.Full;

  return (
    <BottomSheet
      index={state}
      onIndexChange={onStateChange}
      detents={detents}
      // Up to the top of the screen, status bar included: a full-screen player.
      topInset={0}
      wrapNativeView={Animated.createAnimatedComponent}
      onPositionChange={onPositionChange}
      surface={
        <PlayerSurface
          covers={covers}
          queuePosition={queuePosition}
          expansion={expansion}
        />
      }
      surfaceStyle={styles.surface}
    >
      <View style={styles.content}>
        <Animated.View
          style={[
            styles.full,
            { paddingTop: insets.top, paddingBottom: insets.bottom + 24 },
            fullStyle,
          ]}
          pointerEvents={isFull ? 'auto' : 'none'}
          accessibilityElementsHidden={!isFull}
        >
          <View style={styles.topBar}>
            <Pressable
              onPress={() => onStateChange(SheetState.Mini)}
              accessibilityRole="button"
              accessibilityLabel="Collapse player"
              hitSlop={12}
              style={styles.collapse}
            >
              <AppIcon
                name="chevron-right"
                size={26}
                color={colors.textPrimary}
              />
            </Pressable>
            <Text style={styles.topBarTitle}>
              {mood === 'off'
                ? 'Playing from playlist'
                : `${moods[mood].label} mood`}
            </Text>
            <MoodButton mood={mood} onChange={onMoodChange} />
          </View>

          {/* Holds the queue's place; the queue itself is drawn above. */}
          <View style={styles.queueArea} onLayout={onQueueAreaLayout} />

          <View style={styles.nowPlaying}>
            <View style={styles.titles}>
              <Text style={styles.title} numberOfLines={1}>
                {track.title}
              </Text>
              <Text style={styles.artist} numberOfLines={1}>
                {track.artist}
              </Text>
            </View>

            <TrackProgress
              position={position}
              duration={duration}
              onSeek={onSeek}
            />

            <View style={styles.transport}>
              <Pressable
                onPress={onPrevious}
                accessibilityRole="button"
                accessibilityLabel="Previous track"
                hitSlop={12}
              >
                <AppIcon
                  name="skip-back"
                  size={32}
                  color={colors.textPrimary}
                />
              </Pressable>
              <RoundButton
                icon={isPlaying ? 'pause' : 'play'}
                accessibilityLabel={isPlaying ? 'Pause' : 'Play'}
                onPress={onTogglePlayback}
                loading={isLoading}
                size={72}
                color={colors.textPrimary}
                iconColor={colors.background}
              />
              <Pressable
                onPress={onNext}
                accessibilityRole="button"
                accessibilityLabel="Next track"
                hitSlop={12}
              >
                <AppIcon
                  name="skip-forward"
                  size={32}
                  color={colors.textPrimary}
                />
              </Pressable>
            </View>
          </View>
        </Animated.View>

        <Animated.View
          style={[styles.mini, miniStyle]}
          pointerEvents={isFull ? 'none' : 'auto'}
          accessibilityElementsHidden={isFull}
        >
          <Pressable
            onPress={() => onStateChange(SheetState.Full)}
            accessibilityRole="button"
            accessibilityLabel={`${track.title}, ${track.artist}. Open player`}
            style={styles.miniTrack}
          >
            {/* The queue's current cover is drawn here, see `queueStyle`. */}
            <View style={styles.miniCover} />
            <View style={styles.miniTitles}>
              <Text style={styles.miniTitle} numberOfLines={1}>
                {track.title}
              </Text>
              <Text style={styles.miniArtist} numberOfLines={1}>
                {track.artist}
              </Text>
            </View>
          </Pressable>
          <Pressable
            onPress={onTogglePlayback}
            accessibilityRole="button"
            accessibilityLabel={isPlaying ? 'Pause' : 'Play'}
            hitSlop={8}
            style={styles.miniButton}
          >
            {isLoading ? (
              <ActivityIndicator color={colors.textPrimary} />
            ) : (
              <AppIcon
                name={isPlaying ? 'pause' : 'play'}
                size={24}
                color={colors.textPrimary}
              />
            )}
          </Pressable>
          <Pressable
            onPress={onNext}
            accessibilityRole="button"
            accessibilityLabel="Next track"
            hitSlop={8}
            style={styles.miniButton}
          >
            <AppIcon name="skip-forward" size={24} color={colors.textPrimary} />
          </Pressable>
          <MiniProgress position={position} />
        </Animated.View>
        {queueFrame && (
          <Animated.View
            style={[
              styles.queue,
              {
                left: queueFrame.x,
                top: queueFrame.y,
                width: queueFrame.width,
                height: queueFrame.height,
              },
              queueStyle,
            ]}
            pointerEvents={isFull ? 'auto' : 'none'}
            accessibilityElementsHidden={!isFull}
          >
            <QueueCarousel
              tracks={tracks}
              currentIndex={currentIndex}
              position={queuePosition}
              expansion={coverProgress}
              collapsedCoverSize={miniCoverSize}
              onSelect={onSelectTrack}
            />
          </Animated.View>
        )}
      </View>
    </BottomSheet>
  );
});

export default PlayerSheet;

const styles = StyleSheet.create({
  surface: {
    borderRadius: layout.cardRadius,
    borderCurve: 'continuous',
  },
  content: {
    flex: 1,
  },
  mini: {
    height: miniPlayerHeight,
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: miniPaddingHorizontal,
    gap: 4,
  },
  miniTrack: {
    flex: 1,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 10,
  },
  miniCover: {
    width: miniCoverSize,
    height: miniCoverSize,
  },
  queue: {
    position: 'absolute',
    justifyContent: 'center',
  },
  miniTitles: {
    flex: 1,
  },
  miniTitle: {
    color: colors.textPrimary,
    fontSize: 15,
    fontWeight: '600',
  },
  miniArtist: {
    color: colors.textSecondary,
    fontSize: 13,
  },
  miniButton: {
    width: 40,
    height: 40,
    alignItems: 'center',
    justifyContent: 'center',
  },
  miniProgress: {
    position: 'absolute',
    left: 14,
    right: 14,
    bottom: 0,
  },
  full: {
    ...StyleSheet.absoluteFill,
    paddingHorizontal: 24,
  },
  /** The queue takes the height the rest leaves, centred in it. */
  queueArea: {
    flex: 1,
    justifyContent: 'center',
  },
  /** Titles, progress and transport, kept together at the bottom. */
  nowPlaying: {
    gap: 16,
  },
  topBar: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingTop: 12,
  },
  collapse: {
    width: 32,
    transform: [{ rotate: '90deg' }],
  },
  moodButton: {
    width: 32,
    height: 32,
    alignItems: 'center',
    justifyContent: 'center',
  },
  topBarTitle: typography.caption,
  titles: {
    gap: 4,
  },
  title: {
    color: colors.textPrimary,
    fontSize: 24,
    fontWeight: '700',
  },
  artist: {
    color: colors.textSecondary,
    fontSize: 17,
  },
  times: {
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  time: {
    color: colors.textSecondary,
    fontSize: 12,
    fontVariant: ['tabular-nums'],
  },
  transport: {
    marginTop: 8,
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-evenly',
  },
});
