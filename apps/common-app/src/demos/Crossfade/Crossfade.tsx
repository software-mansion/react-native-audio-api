import React, { FC, useEffect, useState, useRef } from 'react';
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
import Animated, {
  SlideInRight,
  SlideOutLeft,
} from 'react-native-reanimated';
import { Heart, SkipBack, SkipForward } from 'lucide-react-native';

import { Container, Spacer } from '../../components';
import PlayPauseIcon from '../../components/icons/PlayPauseIcon';
import { colors } from '../../styles';

const PLAY_DURATION = 3;
const FADE_DURATION = 5;
const ARTWORK_SIZE = Dimensions.get('window').width * 0.7;

const slideOutLeft = SlideOutLeft.duration(FADE_DURATION * 1000);
const slideInRight = SlideInRight.duration(FADE_DURATION * 1000);

const TRACK1 = require('./tracks/track1.mp3');
const TRACK2 = require('./tracks/track2.mp3');

const TRACK_TITLE1 = 'Up-Beat';
const TRACK_TITLE2 = 'Chill';

const COVER1 = require('./images/image_1.jpeg');
const COVER2 = require('./images/image_2.jpg');

const Crossfade: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [trackDuration, setTrackDuration] = useState(0);

  const audioContext = useRef<AudioContext | null>(null);
  const sourceNode1 = useRef<AudioBufferSourceNode | null>(null);
  const sourceNode2 = useRef<AudioBufferSourceNode | null>(null);
  const gainNode1 = useRef<GainNode | null>(null);
  const gainNode2 = useRef<GainNode | null>(null);

  const buffer1 = useRef<AudioBuffer | null>(null);
  const buffer2 = useRef<AudioBuffer | null>(null);

  const [visibleTrack, setVisibleTrack] = useState<1 | 2>(1);
  const [playbackPosition, setPlaybackPosition] = useState(0);

  useEffect(() => {
    const init = async () => {
      audioContext.current = new AudioContext();

      if (buffer1.current && buffer2.current) {
        setIsLoading(false);
        return;
      }

      buffer1.current = await audioContext.current.decodeAudioData(TRACK1);
      buffer2.current = await audioContext.current.decodeAudioData(TRACK2);
      setTrackDuration(buffer1.current.duration);
      setPlaybackPosition(buffer1.current.duration - PLAY_DURATION - FADE_DURATION);

      setIsLoading(false);
    };

    init();

    return () => {
      stopAudio();
      audioContext.current?.suspend();
    };
  }, []);

  const playAudio = async () => {
    if (
      !audioContext.current ||
      !buffer1.current ||
      !buffer2.current ||
      isPlaying
    ) {
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

    // Calculate equal power crossfade curves
    const FADE_STEPS = 150;
    const gain1Curve = new Float32Array(FADE_STEPS);
    const gain2Curve = new Float32Array(FADE_STEPS);

    for (let i = 0; i < FADE_STEPS; i++) {
      const x = i / (FADE_STEPS - 1);
      gain1Curve[i] = Math.cos(x * 0.5 * Math.PI);
      gain2Curve[i] = Math.cos((1.0 - x) * 0.5 * Math.PI);
    }

    // Schedule Track 1
    gainNode1.current.gain.setValueAtTime(1, now);
    gainNode1.current.gain.setValueCurveAtTime(
      gain1Curve,
      now + PLAY_DURATION,
      FADE_DURATION
    );
    sourceNode1.current.onPositionChanged = (event) => {
      if (event.value < buffer1.current!.duration - FADE_DURATION * 0.5) {
        setPlaybackPosition(event.value);
      }
    };
    sourceNode1.current.onEnded = () => {
      if (buffer2.current) {
        setTrackDuration(buffer2.current.duration);
      }
    }
    sourceNode1.current.start(now, buffer1.current.duration - PLAY_DURATION - FADE_DURATION);

    // Schedule Track 2
    gainNode2.current.gain.setValueAtTime(0, now);
    gainNode2.current.gain.setValueCurveAtTime(
      gain2Curve,
      now + PLAY_DURATION,
      FADE_DURATION
    );
    sourceNode2.current.start(now + PLAY_DURATION);
    sourceNode2.current.onPositionChanged = (event) => {
      if (event.value > FADE_DURATION * 0.5) {
        setPlaybackPosition(event.value);
      }
    };

    setVisibleTrack(1);
    setPlaybackPosition(buffer1.current.duration - PLAY_DURATION - FADE_DURATION);

    setTimeout(() => {
      setVisibleTrack(2);
    }, PLAY_DURATION * 1000);

    setIsPlaying(true);
  };

  const stopAudio = async () => {
    if (!isPlaying || !audioContext.current) return;

    setVisibleTrack(1);
    setPlaybackPosition(buffer1.current?.duration ?? 0 - PLAY_DURATION - FADE_DURATION);

    sourceNode1.current?.stop();
    sourceNode2.current?.stop();

    sourceNode1.current?.disconnect();
    sourceNode2.current?.disconnect();
    gainNode1.current?.disconnect();
    gainNode2.current?.disconnect();

    sourceNode1.current = null;
    sourceNode2.current = null;
    gainNode1.current = null;
    gainNode2.current = null;

    await audioContext.current.suspend();
    await AudioManager.setAudioSessionActivity(false);

    setIsPlaying(false);
  };

  const togglePlayPause = () => {
    if (isPlaying) {
      stopAudio();
    } else {
      playAudio();
    }
  };

  const progressPercent = (playbackPosition / trackDuration) * 100;

  const formatTime = (s: number) => {
    const m = Math.floor(s / 60);
    const sec = Math.floor(s % 60);
    return `${m}:${sec.toString().padStart(2, '0')}`;
  };

  return (
    <Container centered>
      {isLoading ? (
        <ActivityIndicator color={colors.white} />
      ) : (
        <View style={styles.content}>

          <View style={styles.artworkContainer}>
            {visibleTrack === 1 && (
              <Animated.View
                exiting={slideOutLeft}
              >
                <Image
                  source={COVER1}
                  style={styles.albumCover}
                  resizeMode="cover"
                />
              </Animated.View>
            )}
            {visibleTrack === 2 && (
              <Animated.View
                entering={slideInRight}
              >
                <Image
                  source={COVER2}
                  style={styles.albumCover}
                  resizeMode="cover"
                />
              </Animated.View>
            )}
          </View>

          <Spacer.Vertical size={40} />

          <View style={styles.trackInfo}>
            <View style={styles.trackInfoText}>
              <Text style={styles.trackTitle} numberOfLines={1}>
                {visibleTrack === 1 ? TRACK_TITLE1 : TRACK_TITLE2}
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
              <Text style={styles.timeText}>{formatTime(playbackPosition)}</Text>
              <Text style={styles.timeText}>{formatTime(trackDuration)}</Text>
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
  title: {
    color: colors.white,
    fontSize: 28,
    fontWeight: 'bold',
    marginBottom: 8,
  },
  description: {
    color: colors.white,
    fontSize: 14,
    opacity: 0.8,
    textAlign: 'center',
  },
  artworkContainer: {
    width: Dimensions.get('window').width,
    height: ARTWORK_SIZE,
    overflow: 'hidden',
    alignItems: 'center',
    justifyContent: 'center',
    display: 'flex',
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
