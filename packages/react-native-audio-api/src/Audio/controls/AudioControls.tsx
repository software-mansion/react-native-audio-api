import React, { useCallback, useMemo, useRef, useState } from 'react';
import {
  ActivityIndicator,
  Animated,
  Image,
  PanResponder,
  Platform,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import type { LayoutChangeEvent } from 'react-native';
import { useAudioTagContext } from '../AudioTagContext';
import {
  formatTime,
  timeFromLocationX,
  useExpandableTrackHeight,
} from './audioControlUtils';

import PlayIcon from './icons/play.png';
import PauseIcon from './icons/pause.png';
import VolumeIcon from './icons/speaker.png';
import MuteIcon from './icons/speaker-x.png';

const TRACK_BAR_HEIGHT = 12;
const TRACK_BAR_HEIGHT_PRESSED = 18;
const TRACK_BAR_ANIM_MS = 150;

const AudioControls: React.FC = () => {
  const {
    ready,
    play,
    pause,
    seekToTime,
    playbackState,
    muted,
    setMuted,
    currentTime,
    duration,
  } = useAudioTagContext();

  const progressTrackAnim = useExpandableTrackHeight(
    TRACK_BAR_HEIGHT,
    TRACK_BAR_HEIGHT_PRESSED,
    TRACK_BAR_ANIM_MS
  );

  const [scrubTime, setScrubTime] = useState<number | null>(null);

  const scrub = useRef({
    startX: 0,
    trackWidth: 0,
    duration,
    seekToTime,
    expand: progressTrackAnim.expand,
    collapse: progressTrackAnim.collapse,
  });
  scrub.current.duration = duration;
  scrub.current.seekToTime = seekToTime;
  scrub.current.expand = progressTrackAnim.expand;
  scrub.current.collapse = progressTrackAnim.collapse;

  const panResponder = useMemo(
    () =>
      PanResponder.create({
        onStartShouldSetPanResponder: () => true,
        onMoveShouldSetPanResponder: () => true,
        onPanResponderTerminationRequest: () => false,
        onPanResponderGrant: (event) => {
          const s = scrub.current;
          s.startX = event.nativeEvent.locationX;
          s.expand();
          setScrubTime(timeFromLocationX(s.startX, s.trackWidth, s.duration));
        },
        onPanResponderMove: (_event, gestureState) => {
          const s = scrub.current;
          setScrubTime(
            timeFromLocationX(
              s.startX + gestureState.dx,
              s.trackWidth,
              s.duration
            )
          );
        },
        onPanResponderRelease: (_event, gestureState) => {
          const s = scrub.current;
          s.seekToTime(
            timeFromLocationX(
              s.startX + gestureState.dx,
              s.trackWidth,
              s.duration
            )
          );
          s.collapse();
          setScrubTime(null);
        },
        onPanResponderTerminate: () => {
          scrub.current.collapse();
          setScrubTime(null);
        },
      }),
    []
  );

  const onPlayPausePress = useCallback(() => {
    if (playbackState === 'playing') {
      pause();
    } else {
      play();
    }
  }, [playbackState, pause, play]);

  const onProgressTrackLayout = useCallback((event: LayoutChangeEvent) => {
    scrub.current.trackWidth = event.nativeEvent.layout.width;
  }, []);

  if (!ready) {
    return (
      <View style={styles.container}>
        <ActivityIndicator color="#333" size="small" />
      </View>
    );
  }

  const displayTime = scrubTime ?? currentTime;
  const progress = duration > 0 ? displayTime / duration : 0;

  return (
    <View style={styles.container}>
      <View style={styles.topRow}>
        <Pressable style={styles.playPause} onPress={onPlayPausePress}>
          {playbackState === 'playing' ? (
            <Image source={PauseIcon} style={styles.icon} />
          ) : (
            <Image source={PlayIcon} style={styles.icon} />
          )}
        </Pressable>

        <Text style={styles.timeText}>
          {formatTime(displayTime)} / {formatTime(duration)}
        </Text>

        {/* prettier-ignore */}
        <View
          onLayout={onProgressTrackLayout}
          style={styles.progressTrack}
          {...panResponder.panHandlers}>
          <Animated.View
            style={[styles.trackInner, progressTrackAnim.animatedStyle]}>
            <View style={[styles.trackFill, { width: `${progress * 100}%` }]} />
          </Animated.View>
        </View>

        <Pressable style={styles.volumeIcon} onPress={() => setMuted(!muted)}>
          {muted ? (
            <Image source={MuteIcon} style={styles.icon} />
          ) : (
            <Image source={VolumeIcon} style={styles.icon} />
          )}
        </Pressable>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flexDirection: 'column',
    alignSelf: 'stretch',
    minWidth: 200,
    paddingVertical: 10,
    paddingHorizontal: 12,
    backgroundColor: '#f5f5f5',
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
    ...Platform.select({
      ios: {
        shadowColor: '#000',
        shadowOffset: { width: 0, height: 2 },
        shadowOpacity: 0.15,
        shadowRadius: 4,
      },
      android: {
        elevation: 4,
      },
    }),
  },
  topRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  icon: {
    width: 24,
    height: 24,
  },
  playPause: {
    padding: 4,
    marginRight: 12,
  },
  timeText: {
    color: '#000',
    fontSize: 12,
    marginRight: 10,
    minWidth: 48,
  },
  progressTrack: {
    flex: 1,
    minWidth: 40,
    justifyContent: 'center',
    marginRight: 10,
  },
  trackInner: {
    alignSelf: 'stretch',
    backgroundColor: '#ccc',
    overflow: 'hidden',
  },
  trackFill: {
    height: '100%',
    backgroundColor: '#000',
  },
  volumeIcon: {
    padding: 4,
    marginRight: 12,
  },
  loadingRow: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  loadingText: {
    color: '#333',
    fontSize: 14,
  },
});

export default AudioControls;
