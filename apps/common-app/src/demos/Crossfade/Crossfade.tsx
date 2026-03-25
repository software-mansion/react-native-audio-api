import React, { FC, useCallback, useEffect, useState, useRef } from 'react';
import {
  AudioBuffer,
  AudioManager,
  AudioBufferSourceNode,
  GainNode,
  AudioContext,
} from 'react-native-audio-api';
import {
  StyleSheet,
  Text,
  View,
  Image,
  ActivityIndicator,
  Dimensions,
  Pressable,
} from 'react-native';
import { GestureDetector, Gesture } from 'react-native-gesture-handler';
import Animated, {
  useSharedValue,
  useAnimatedStyle,
  useAnimatedReaction,
  withSpring,
} from 'react-native-reanimated';
import { Heart, SkipBack, SkipForward } from 'lucide-react-native';
import { scheduleOnRN } from 'react-native-worklets';

import { Container, Spacer } from '../../components';
import PlayPauseIcon from '../../components/icons/PlayPauseIcon';
import { colors } from '../../styles';

const ARTWORK_SIZE = Dimensions.get('window').width * 0.7;
const TILE_OFFSET = 60;
const TILE_DISTANCE = ARTWORK_SIZE + TILE_OFFSET;
const MAX_GAIN = 0.5;

const TRACKS = [
  { title: 'Up-Beat', cover: require('./images/image_1.jpeg'), uri: require('./tracks/track1.mp3') },
  { title: 'Chill', cover: require('./images/image_2.jpg'), uri: require('./tracks/track2.mp3') },
] as const;

function equalPowerGain1(progress: number): number {
  return Math.cos(progress * 0.5 * Math.PI) * MAX_GAIN;
}

function equalPowerGain2(progress: number): number {
  return Math.cos((1 - progress) * 0.5 * Math.PI) * MAX_GAIN;
}

function formatTime(seconds: number): string {
  const m = Math.floor(seconds / 60);
  const s = Math.floor(seconds % 60);
  return `${m}:${s.toString().padStart(2, '0')}`;
}

const Crossfade: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [trackDuration, setTrackDuration] = useState(0);
  const [visibleTrack, setVisibleTrack] = useState<1 | 2>(1);
  const [playbackPosition, setPlaybackPosition] = useState(0);

  const audioContext = useRef<AudioContext | null>(null);
  const sourceNode1 = useRef<AudioBufferSourceNode | null>(null);
  const sourceNode2 = useRef<AudioBufferSourceNode | null>(null);
  const gainNode1 = useRef<GainNode | null>(null);
  const gainNode2 = useRef<GainNode | null>(null);
  const buffer1 = useRef<AudioBuffer | null>(null);
  const buffer2 = useRef<AudioBuffer | null>(null);
  const visibleTrackRef = useRef<1 | 2>(1);

  const progress = useSharedValue(0);
  const swipeStartProgress = useSharedValue(0);
  const isPlayingShared = useSharedValue(0);
  const swipeEndSnapTo = useSharedValue(-1);

  useEffect(() => {
    isPlayingShared.value = isPlaying ? 1 : 0;
    visibleTrackRef.current = visibleTrack;
  }, [isPlaying, visibleTrack, isPlayingShared]);

  const applyGainFromProgress = useCallback((p: number) => {
    const ctx = audioContext.current;
    const g1 = gainNode1.current;
    const g2 = gainNode2.current;
    if (!ctx || !g1 || !g2) {
      return;
    }
    const now = ctx.currentTime;
    g1.gain.setValueAtTime(equalPowerGain1(p), now);
    g2.gain.setValueAtTime(equalPowerGain2(p), now);
  }, []);

  const commitSwipeEnd = useCallback((snapTo: number) => {
    const track: 1 | 2 = snapTo < 0.5 ? 1 : 2;
    setVisibleTrack(track);
    visibleTrackRef.current = track;

    const buf = track === 1 ? buffer1.current : buffer2.current;
    if (buf) {
      setTrackDuration(buf.duration);
    }

    const ctx = audioContext.current;
    const g1 = gainNode1.current;
    const g2 = gainNode2.current;
    if (!ctx || !g1 || !g2) {
      return;
    }
    const now = ctx.currentTime;
    g1.gain.setValueAtTime(visibleTrack === 1 ? MAX_GAIN : 0, now);
    g2.gain.setValueAtTime(visibleTrack !== 1 ? MAX_GAIN : 0, now);
  }, []);

  useAnimatedReaction(
    () => progress.value,
    (p) => {
      if (isPlayingShared.value === 1) {
        scheduleOnRN(applyGainFromProgress, p);
      }
    },
  );

  useAnimatedReaction(
    () => swipeEndSnapTo.value,
    (snapTo) => {
      if (snapTo >= 0) {
        scheduleOnRN(commitSwipeEnd, snapTo);
        swipeEndSnapTo.value = -1;
      }
    },
  );

  useEffect(() => {
    const init = async () => {
      audioContext.current = new AudioContext();

      if (buffer1.current && buffer2.current) {
        setIsLoading(false);
        return;
      }

      buffer1.current = await audioContext.current.decodeAudioData(TRACKS[0].uri);
      buffer2.current = await audioContext.current.decodeAudioData(TRACKS[1].uri);
      setTrackDuration(buffer1.current.duration);
      setPlaybackPosition(0);
      setIsLoading(false);
    };

    init();

    return () => {
      stopAudio();
      audioContext.current?.suspend();
    };
  }, []);

  const playAudio = useCallback(async () => {
    if (!audioContext.current || !buffer1.current || !buffer2.current || isPlaying) {
      return;
    }

    AudioManager.setAudioSessionOptions({
      iosCategory: 'playback',
      iosMode: 'default',
      iosOptions: [],
    });
    await AudioManager.setAudioSessionActivity(true);

    if (audioContext.current.state === 'suspended') {
      await audioContext.current.resume();
    }

    sourceNode1.current = audioContext.current.createBufferSource();
    sourceNode1.current.buffer = buffer1.current;
    sourceNode2.current = audioContext.current.createBufferSource();
    sourceNode2.current.buffer = buffer2.current;
    gainNode1.current = audioContext.current.createGain();
    gainNode2.current = audioContext.current.createGain();

    sourceNode1.current
      .connect(gainNode1.current)
      .connect(audioContext.current.destination);
    sourceNode2.current
      .connect(gainNode2.current)
      .connect(audioContext.current.destination);

    const now = audioContext.current.currentTime;
    const maxOffset = Math.max(
      0,
      Math.min(buffer1.current.duration, buffer2.current.duration) - 0.01,
    );
    const startOffset = Math.min(Math.max(0, playbackPosition), maxOffset);

    gainNode1.current.gain.setValueAtTime(visibleTrack === 1 ? MAX_GAIN : 0, now);
    gainNode2.current.gain.setValueAtTime(visibleTrack !== 1 ? MAX_GAIN : 0, now);
    progress.value = visibleTrack === 1 ? 0 : 1;

    sourceNode1.current.onPositionChanged = (event) => {
      if (visibleTrackRef.current === 1) {
        setPlaybackPosition(event.value);
      }
    };
    sourceNode1.current.start(now, startOffset);

    sourceNode2.current.onPositionChanged = (event) => {
      if (visibleTrackRef.current === 2) {
        setPlaybackPosition(event.value);
      }
    };
    sourceNode2.current.start(now, startOffset);

    setIsPlaying(true);
  }, [isPlaying, visibleTrack, playbackPosition]);

  const stopAudio = useCallback(async () => {
    if (!isPlaying || !audioContext.current) {
      return;
    }

    gainNode1.current?.disconnect();
    gainNode2.current?.disconnect();
    sourceNode1.current = null;
    sourceNode2.current = null;
    gainNode1.current = null;
    gainNode2.current = null;

    await audioContext.current.suspend();
    await AudioManager.setAudioSessionActivity(false);
    setIsPlaying(false);
  }, [isPlaying]);

  const togglePlayPause = useCallback(() => {
    if (isPlaying) {
      stopAudio();
    } else {
      playAudio();
    }
  }, [isPlaying, playAudio, stopAudio]);

  const progressPercent =
    trackDuration > 0 ? (playbackPosition / trackDuration) * 100 : 0;

  const panGesture = Gesture.Pan()
    .activeOffsetX([-20, 20])
    .onStart(() => {
      swipeStartProgress.value = progress.value;
    })
    .onUpdate((event) => {
      const p = Math.max(
        0,
        Math.min(1, swipeStartProgress.value - event.translationX / ARTWORK_SIZE),
      );
      progress.value = p;
    })
    .onEnd(() => {
      const p = progress.value;
      const snapTo = p < 0.5 ? 0 : 1;
      progress.value = withSpring(snapTo, {
        damping: 100,
        stiffness: 300,
      });
      swipeEndSnapTo.value = snapTo;
    });

  const track1TileStyle = useAnimatedStyle(() => {
    const p = progress.value;
    return {
      transform: [{ translateX: -p * TILE_DISTANCE }],
    };
  });

  const track2TileStyle = useAnimatedStyle(() => {
    const p = progress.value;
    return {
      transform: [{ translateX: (1 - p) * TILE_DISTANCE }],
    };
  });

  const currentTrackTitle = TRACKS[visibleTrack - 1].title;

  return (
    <Container centered>
      {isLoading ? (
        <ActivityIndicator color={colors.white} />
      ) : (
        <View style={styles.content}>
          <View style={styles.artworkWrapper}>
            <GestureDetector gesture={panGesture}>
              <View style={styles.artworkContainer}>
                <Animated.View style={[styles.tile, track1TileStyle]}>
                  <Image
                    source={TRACKS[0].cover}
                    style={styles.albumCover}
                    resizeMode="cover"
                  />
                </Animated.View>
                <Animated.View style={[styles.tile, track2TileStyle]}>
                  <Image
                    source={TRACKS[1].cover}
                    style={styles.albumCover}
                    resizeMode="cover"
                  />
                </Animated.View>
              </View>
            </GestureDetector>
          </View>

          <Spacer.Vertical size={40} />

          <View style={styles.trackInfo}>
            <View style={styles.trackInfoText}>
              <Text style={styles.trackTitle} numberOfLines={1}>
                {currentTrackTitle}
              </Text>
              <Text style={styles.trackArtist} numberOfLines={1}>
                Lo-Fi Boy
              </Text>
            </View>
            <Heart
              color={colors.white}
              size={24}
              fill="transparent"
              style={styles.heartIcon}
            />
          </View>

          <Spacer.Vertical size={30} />

          <View style={styles.progressSection}>
            <View style={styles.progressBar}>
              <View
                style={[styles.progressFill, { width: `${progressPercent}%` }]}
              />
            </View>
            <View style={styles.timeRow}>
              <Text style={styles.timeText}>
                {formatTime(playbackPosition)}
              </Text>
              <Text style={styles.timeText}>
                {formatTime(trackDuration)}
              </Text>
            </View>
          </View>

          <Spacer.Vertical size={20} />

          <View style={styles.controlsRow}>
            <Pressable
              onPress={() => {}}
              style={({ pressed }) => [
                styles.iconButton,
                pressed && styles.iconButtonPressed,
              ]}
            >
              <SkipBack color={colors.white} size={32} />
            </Pressable>
            <Pressable
              onPress={togglePlayPause}
              style={({ pressed }) => [
                styles.iconButton,
                pressed && styles.iconButtonPressed,
              ]}
            >
              <PlayPauseIcon
                isPlaying={isPlaying}
                size={56}
                color={colors.white}
              />
            </Pressable>
            <Pressable
              onPress={() => {}}
              style={({ pressed }) => [
                styles.iconButton,
                pressed && styles.iconButtonPressed,
              ]}
            >
              <SkipForward color={colors.white} size={32} />
            </Pressable>
          </View>
        </View>
      )}
    </Container>
  );
};

export default Crossfade;

const styles = StyleSheet.create({
  content: {
    width: '100%',
    alignItems: 'center',
    paddingHorizontal: 20,
  },
  artworkWrapper: {
    width: ARTWORK_SIZE + 48,
    height: ARTWORK_SIZE + 48,
    borderRadius: 12,
    backgroundColor: `${colors.main}12`,
    alignItems: 'center',
    justifyContent: 'center',
  },
  artworkContainer: {
    width: ARTWORK_SIZE,
    height: ARTWORK_SIZE,
    overflow: 'hidden',
    position: 'relative',
  },
  tile: {
    position: 'absolute',
    left: 0,
    top: 0,
    width: ARTWORK_SIZE,
    height: ARTWORK_SIZE,
  },
  albumCover: {
    width: ARTWORK_SIZE,
    height: ARTWORK_SIZE,
  },
  trackInfo: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    width: ARTWORK_SIZE,
    marginTop: 12,
    paddingHorizontal: 4,
  },
  trackInfoText: {
    flex: 1,
    marginRight: 8,
  },
  trackTitle: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '600',
  },
  trackArtist: {
    color: colors.white,
    fontSize: 14,
    opacity: 0.8,
    marginTop: 2,
  },
  heartIcon: {
    opacity: 0.9,
  },
  progressSection: {
    width: '100%',
    maxWidth: ARTWORK_SIZE,
  },
  progressBar: {
    height: 6,
    backgroundColor: colors.separator,
    borderRadius: 3,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: colors.main,
    borderRadius: 3,
  },
  timeRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginTop: 6,
  },
  timeText: {
    color: colors.white,
    fontSize: 12,
    opacity: 0.9,
  },
  controlsRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    gap: 32,
  },
  iconButton: {
    padding: 12,
  },
  iconButtonPressed: {
    opacity: 0.7,
  },
});
