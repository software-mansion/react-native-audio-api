import {
  isLiquidGlassSupported,
  LiquidGlassView,
} from '@callstack/liquid-glass';
import { useNavigation } from '@react-navigation/native';
import React, {
  FC,
  useCallback,
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';
import {
  ActivityIndicator,
  Pressable,
  StyleSheet,
  Text,
  useWindowDimensions,
  View,
} from 'react-native';
import { GestureDetector, usePanGesture } from 'react-native-gesture-handler';
import {
  Easing,
  useAnimatedReaction,
  useSharedValue,
  withSpring,
  withTiming,
} from 'react-native-reanimated';
import { scheduleOnRN } from 'react-native-worklets';

import { AppIcon, Container } from '../../components';
import { colors } from '../../styles';
import ArtworkStack from './ArtworkStack';
import CoverBackdrop from './CoverBackdrop';
import CrossfadePlayer from './CrossfadePlayer';
import CrossfadeCapsule from './CrossfadeCapsule';
import { TRACKS } from './tracks';

/** A skip button crossfades over this long while playing. */
const SKIP_CROSSFADE_MS = 3000;
/** While paused there's nothing to hear, so a skip only swaps the covers. */
const SKIP_SWAP_MS = 350;
/** A flick this fast (pt/s) moves on a track even if the swipe was short. */
const FLICK_VELOCITY = 450;
/** Share of the artwork width a swipe has to travel to move on a track. */
const COMMIT_SHARE = 0.3;
/** The swipe starts after this much horizontal travel (pt). */
const SWIPE_ACTIVATION = 10;
/** Vertical travel (pt) that rules a touch out as a swipe. */
const SWIPE_VERTICAL_TOLERANCE = 24;
/** The swipe area reaches this far above and below the artwork and title. */
const SWIPE_AREA_OUTSET = 12;
/** Gap between the screen edges and the player. */
const SCREEN_MARGIN = 36;
/** The title, progress and times sit this far inside the artwork's edges. */
const TEXT_INSET = 8;
const PLAY_BUTTON_SIZE = 66;
const HEART_BUTTON_SIZE = 40;

const LAST_TRACK = TRACKS.length - 1;
const COVERS = TRACKS.map((track) => track.cover);

function formatTime(seconds: number): string {
  const whole = Math.max(0, Math.floor(seconds));
  return `${Math.floor(whole / 60)}:${String(whole % 60).padStart(2, '0')}`;
}

/**
 * A playlist you move through by swiping the artwork (or with the skip
 * buttons). `progress` is the index of the current track; while it's between
 * two indices both tracks play, crossfaded along an equal-power curve.
 */
const Crossfade: FC = () => {
  const { width } = useWindowDimensions();
  const navigation = useNavigation('Crossfade');

  // On iOS 26 the back gesture works anywhere on the screen, so a swipe to the
  // previous track would pop the screen. Keep back to the edge swipe.
  useLayoutEffect(() => {
    navigation.setOptions({ fullScreenGestureEnabled: false });
  }, [navigation]);
  const playerWidth = width - 2 * SCREEN_MARGIN;
  const artworkSize = playerWidth - 2 * TEXT_INSET;

  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTrack, setCurrentTrack] = useState(0);
  const [destinationTrack, setDestinationTrack] = useState(0);
  const [durations, setDurations] = useState<Record<number, number>>({});
  const [playbackPosition, setPlaybackPosition] = useState(0);
  const [likedTracks, setLikedTracks] = useState<Set<number>>(new Set());

  const player = useRef<CrossfadePlayer | null>(null);

  /** Index of the current track; fractions are mid-crossfade. */
  const progress = useSharedValue(0);
  /** Where `progress` is heading, for the "Crossfading to …" capsule. */
  const destination = useSharedValue(0);
  const swipeStartTrack = useSharedValue(0);

  useEffect(() => {
    const instance = new CrossfadePlayer(TRACKS, {
      onPosition: setPlaybackPosition,
      onTrackLoaded: (index, duration) =>
        setDurations((loaded) => ({ ...loaded, [index]: duration })),
    });
    player.current = instance;
    instance.prepareAround(0);

    return () => {
      player.current = null;
      instance.close();
    };
  }, []);

  useEffect(() => {
    player.current?.prepareAround(currentTrack);
  }, [currentTrack]);

  const setMix = useCallback((mix: number) => {
    player.current?.setMix(mix);
  }, []);

  useAnimatedReaction(
    () => progress.value,
    (mix: number) => {
      scheduleOnRN(setMix, mix);
    }
  );

  useAnimatedReaction(
    () => destination.value,
    (track: number) => {
      scheduleOnRN(setDestinationTrack, track);
    }
  );

  const play = useCallback(async () => {
    setIsPlaying(true);
    await player.current?.play();
  }, []);

  const pause = useCallback(async () => {
    setIsPlaying(false);
    await player.current?.pause();
  }, []);

  const crossfadeTo = useCallback(
    (track: number) => {
      if (track < 0 || track > LAST_TRACK || track === currentTrack) {
        return;
      }
      destination.value = track;
      progress.value = withTiming(track, {
        duration: isPlaying ? SKIP_CROSSFADE_MS : SKIP_SWAP_MS,
        easing: Easing.inOut(Easing.quad),
      });
      setCurrentTrack(track);
    },
    [currentTrack, destination, isPlaying, progress]
  );

  const panGesture = usePanGesture({
    activeOffsetX: [-SWIPE_ACTIVATION, SWIPE_ACTIVATION],
    failOffsetY: [-SWIPE_VERTICAL_TOLERANCE, SWIPE_VERTICAL_TOLERANCE],
    onActivate: () => {
      swipeStartTrack.value = Math.round(progress.value);
    },
    onUpdate: (event) => {
      // One swipe moves at most one track either way.
      const start = swipeStartTrack.value;
      const next = start - event.translationX / artworkSize;
      progress.value = Math.min(
        Math.min(LAST_TRACK, start + 1),
        Math.max(Math.max(0, start - 1), next)
      );
      destination.value =
        progress.value >= start
          ? Math.min(LAST_TRACK, start + 1)
          : Math.max(0, start - 1);
    },
    onDeactivate: (event) => {
      const start = swipeStartTrack.value;
      const travelled = progress.value - start;
      const direction = Math.sign(travelled);
      // Swiping left (negative velocity) moves forward through the list.
      const flickDirection = -Math.sign(event.velocityX);
      const speed = Math.abs(event.velocityX);
      const isFlickForward =
        speed > FLICK_VELOCITY &&
        (direction === 0 || flickDirection === direction);
      // Cancelling takes a clearly deliberate flick back, so a slow drag that
      // jitters backwards as the finger lifts still lands.
      const isFlickBack =
        speed > 2 * FLICK_VELOCITY && flickDirection === -direction;
      let track = start;
      if (isFlickForward) {
        track = start + flickDirection;
      } else if (!isFlickBack && Math.abs(travelled) > COMMIT_SHARE) {
        track = start + direction;
      }
      track = Math.min(LAST_TRACK, Math.max(0, track));
      destination.value = track;
      progress.value = withSpring(track, { damping: 100, stiffness: 300 });
      scheduleOnRN(setCurrentTrack, track);
    },
  });

  const duration = durations[currentTrack] ?? 0;
  const isCurrentTrackLoaded = duration > 0;
  const position = isCurrentTrackLoaded ? playbackPosition % duration : 0;
  const elapsedShare = isCurrentTrackLoaded ? position / duration : 0;
  const track = TRACKS[currentTrack];
  const isLiked = likedTracks.has(currentTrack);

  const toggleLiked = () =>
    setLikedTracks((liked) => {
      const next = new Set(liked);
      if (!next.delete(currentTrack)) {
        next.add(currentTrack);
      }
      return next;
    });

  return (
    <View style={styles.root}>
      <CoverBackdrop covers={COVERS} progress={progress} />
      <Container centered style={styles.container}>
        <View style={[styles.player, { width: playerWidth }]}>
          <GestureDetector gesture={panGesture}>
            {/* Swipes start anywhere across the screen over the artwork and title. */}
            <View
              style={[
                styles.swipeArea,
                {
                  marginHorizontal: -SCREEN_MARGIN,
                  paddingHorizontal: SCREEN_MARGIN,
                },
              ]}
            >
              <ArtworkStack
                covers={COVERS}
                progress={progress}
                size={artworkSize}
              />

              <View style={styles.titleRow}>
                <View style={styles.titles}>
                  <Text style={styles.title} numberOfLines={1}>
                    {track.title}
                  </Text>
                  <Text style={styles.artist} numberOfLines={1}>
                    {track.artist}
                  </Text>
                </View>
                <Pressable
                  onPress={toggleLiked}
                  accessibilityRole="button"
                  accessibilityLabel="Favourite"
                  accessibilityState={{ selected: isLiked }}
                >
                  <LiquidGlassView
                    interactive
                    colorScheme="dark"
                    style={[
                      styles.heartButton,
                      !isLiquidGlassSupported && styles.glassFallback,
                    ]}
                  >
                    <AppIcon
                      name="heart"
                      size={19}
                      color={isLiked ? colors.accent : colors.textPrimary}
                    />
                  </LiquidGlassView>
                </Pressable>
              </View>
            </View>
          </GestureDetector>

          <View>
            <View style={styles.progressTrack}>
              <View
                style={[
                  styles.progressFill,
                  { width: `${elapsedShare * 100}%` },
                ]}
              />
            </View>
            <View style={styles.times}>
              <Text style={styles.time}>{formatTime(position)}</Text>
              <Text style={styles.time}>
                −{formatTime(duration - position)}
              </Text>
            </View>
          </View>

          <View style={styles.controls}>
            <Pressable
              onPress={() => crossfadeTo(currentTrack - 1)}
              disabled={currentTrack === 0}
              accessibilityRole="button"
              accessibilityLabel="Previous track"
              style={({ pressed }) => [
                styles.skipButton,
                currentTrack === 0 && styles.disabled,
                pressed && styles.pressed,
              ]}
            >
              <AppIcon name="skip-back" size={30} color={colors.white} />
            </Pressable>
            <Pressable
              onPress={isPlaying ? pause : play}
              accessibilityRole="button"
              accessibilityLabel={isPlaying ? 'Pause' : 'Play'}
              style={({ pressed }) => [
                styles.playButton,
                pressed && styles.pressed,
              ]}
            >
              {isPlaying && !isCurrentTrackLoaded ? (
                <ActivityIndicator color={colors.background} />
              ) : (
                <AppIcon
                  name={isPlaying ? 'pause' : 'play'}
                  size={28}
                  color={colors.background}
                />
              )}
            </Pressable>
            <Pressable
              onPress={() => crossfadeTo(currentTrack + 1)}
              disabled={currentTrack === LAST_TRACK}
              accessibilityRole="button"
              accessibilityLabel="Next track"
              style={({ pressed }) => [
                styles.skipButton,
                currentTrack === LAST_TRACK && styles.disabled,
                pressed && styles.pressed,
              ]}
            >
              <AppIcon name="skip-forward" size={30} color={colors.white} />
            </Pressable>
          </View>

          <CrossfadeCapsule
            progress={progress}
            destination={destination}
            destinationTitle={TRACKS[destinationTrack].title}
          />
        </View>
      </Container>
    </View>
  );
};

export default Crossfade;

const styles = StyleSheet.create({
  root: {
    flex: 1,
    backgroundColor: colors.background,
  },
  container: {
    backgroundColor: 'transparent',
  },
  player: {
    gap: 18,
  },
  swipeArea: {
    gap: 18,
    marginVertical: -SWIPE_AREA_OUTSET,
    paddingVertical: SWIPE_AREA_OUTSET,
  },
  titleRow: {
    marginTop: 14,
    paddingHorizontal: TEXT_INSET,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  titles: {
    flex: 1,
  },
  title: {
    fontSize: 21,
    fontWeight: '700',
    color: colors.textPrimary,
  },
  artist: {
    marginTop: 2,
    fontSize: 15,
    color: 'rgba(238, 240, 255, 0.65)',
  },
  heartButton: {
    width: HEART_BUTTON_SIZE,
    height: HEART_BUTTON_SIZE,
    borderRadius: HEART_BUTTON_SIZE / 2,
    alignItems: 'center',
    justifyContent: 'center',
  },
  glassFallback: {
    backgroundColor: 'rgba(255, 255, 255, 0.12)',
  },
  progressTrack: {
    height: 4,
    marginHorizontal: TEXT_INSET,
    borderRadius: 2,
    overflow: 'hidden',
    backgroundColor: 'rgba(255, 255, 255, 0.2)',
  },
  progressFill: {
    height: '100%',
    backgroundColor: colors.textPrimary,
  },
  times: {
    marginTop: 6,
    marginHorizontal: TEXT_INSET,
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  time: {
    fontSize: 12,
    fontVariant: ['tabular-nums'],
    color: 'rgba(238, 240, 255, 0.65)',
  },
  controls: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 38,
  },
  skipButton: {
    padding: 8,
  },
  playButton: {
    width: PLAY_BUTTON_SIZE,
    height: PLAY_BUTTON_SIZE,
    borderRadius: PLAY_BUTTON_SIZE / 2,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: colors.textPrimary,
  },
  disabled: {
    opacity: 0.35,
  },
  pressed: {
    opacity: 0.7,
    transform: [{ scale: 0.96 }],
  },
});
