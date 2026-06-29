import React, { useCallback, useMemo, useRef, useState } from 'react';
import {
  ActivityIndicator,
  Image,
  Platform,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import { Gesture, GestureDetector } from 'react-native-gesture-handler';
import Animated, {
  useAnimatedRef,
  useSharedValue,
} from 'react-native-reanimated';
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
const SCRUB_PAN_MIN_DISTANCE = 8;

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
  const progressTrackRef = useAnimatedRef<View>();
  const progressMetricsWidth = useSharedValue(0);
  const durationRef = useRef(duration);
  durationRef.current = duration;

  const onStart = useCallback(
    (x: number) => {
      progressTrackAnim.expand();
      const d = durationRef.current;
      progressTrackRef.current?.measureInWindow((_left, _y, width, _h) => {
        progressMetricsWidth.value = width;
        setScrubTime(timeFromLocationX(x, width, d));
      });
    },
    [progressTrackAnim, progressMetricsWidth, setScrubTime, progressTrackRef]
  );

  const onUpdate = useCallback(
    (x: number) => {
      const d = durationRef.current;
      const w = progressMetricsWidth.value;
      setScrubTime(timeFromLocationX(x, w, d));
    },
    [progressMetricsWidth]
  );

  const seekTo = useCallback(
    (x: number) => {
      const d = durationRef.current;
      const w = progressMetricsWidth.value;
      const t = timeFromLocationX(x, w, d);
      seekToTime(t);
    },
    [progressMetricsWidth, seekToTime]
  );

  const onEnd = useCallback(
    (x: number) => {
      seekTo(x);
      progressTrackAnim.collapse();
      setScrubTime(null);
    },
    [progressTrackAnim, seekTo]
  );

  const onCancel = useCallback(() => {
    progressTrackAnim.collapse();
    setScrubTime(null);
  }, [progressTrackAnim]);

  const onTapSeek = useCallback(
    (x: number) => {
      onEnd(x);
    },
    [onEnd]
  );

  const scrubGesture = useMemo(() => {
    const panGesture = Gesture.Pan()
      .runOnJS(true)
      .minDistance(SCRUB_PAN_MIN_DISTANCE)
      .onStart((e) => {
        onStart(e.x);
      })
      .onUpdate((e) => {
        onUpdate(e.x);
      })
      .onEnd((e) => {
        onEnd(e.x);
      })
      .onFinalize((_e, success) => {
        if (!success) {
          onCancel();
        }
      });

    const tapGesture = Gesture.Tap()
      .runOnJS(true)
      .maxDistance(14)
      .onEnd((e, success) => {
        if (success) {
          onTapSeek(e.x);
        }
      });

    return Gesture.Race(panGesture, tapGesture);
  }, [onStart, onUpdate, onEnd, onCancel, onTapSeek]);

  const onPlayPausePress = useCallback(() => {
    if (playbackState === 'playing') {
      pause();
    } else {
      play();
    }
  }, [playbackState, pause, play]);

  const onProgressTrackLayout = useCallback(() => {
    progressTrackRef.current?.measureInWindow((_left, _y, width, _h) => {
      progressMetricsWidth.value = width;
    });
  }, [progressMetricsWidth, progressTrackRef]);

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
            <Image source={PauseIcon} style={{ width: 24, height: 24 }} />
          ) : (
            <Image source={PlayIcon} style={{ width: 24, height: 24 }} />
          )}
        </Pressable>

        <Text style={styles.timeText}>
          {formatTime(displayTime)} / {formatTime(duration)}
        </Text>

        <GestureDetector gesture={scrubGesture}>
          {/* prettier-ignore */}
          <View
            ref={progressTrackRef}
            onLayout={onProgressTrackLayout}
            style={styles.progressTrack}
            collapsable={false}>
            <Animated.View
              style={[styles.trackInner, progressTrackAnim.animatedStyle]}
              >
              <View
                style={[styles.trackFill, { width: `${progress * 100}%` }]}
              />
            </Animated.View>
          </View>
        </GestureDetector>

        <Pressable style={styles.volumeIcon} onPress={() => setMuted(!muted)}>
          {muted ? (
            <Image source={MuteIcon} style={{ width: 24, height: 24 }} />
          ) : (
            <Image source={VolumeIcon} style={{ width: 24, height: 24 }} />
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
