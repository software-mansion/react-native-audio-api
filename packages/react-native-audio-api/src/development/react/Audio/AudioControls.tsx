import React, { useCallback, useMemo, useRef, useState } from 'react';
import {
  ActivityIndicator,
  Image,
  LayoutChangeEvent,
  Platform,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import {
  Gesture,
  GestureDetector,
  GestureHandlerRootView,
} from 'react-native-gesture-handler';
import { scheduleOnRN } from 'react-native-worklets';
import Animated, {
  useAnimatedStyle,
  useSharedValue,
  withTiming,
} from 'react-native-reanimated';
import { useAudioTagContext } from './AudioTagContext';

import PlayIcon from './icons/play.png';
import PauseIcon from './icons/pause.png';
import VolumeIcon from './icons/speaker.png';
import MuteIcon from './icons/speaker-x.png';

function formatTime(seconds: number): string {
  if (!Number.isFinite(seconds) || seconds < 0) return '0:00';
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = Math.floor(seconds % 60);
  if (h > 0) {
    return `${h}:${m.toString().padStart(2, '0')}:${s.toString().padStart(2, '0')}`;
  } else {
    return `${m}:${s.toString().padStart(2, '0')}`;
  }
}

function timeFromLocationX(
  locationX: number,
  trackWidth: number,
  durationSeconds: number
): number {
  if (trackWidth <= 0 || durationSeconds <= 0) {
    return 0;
  }
  const pct = Math.max(0, Math.min(1, locationX / trackWidth));
  return pct * durationSeconds;
}

const TRACK_BAR_HEIGHT = 12;
const TRACK_BAR_HEIGHT_PRESSED = 18;
const TRACK_BAR_ANIM_MS = 150;
const NATIVE_DELAY_MS = 10; // delay between native calls so audio engine can catch up
const SCRUB_PAN_MIN_DISTANCE = 8;

function useExpandableTrackHeight() {
  const height = useSharedValue(TRACK_BAR_HEIGHT);
  const animatedStyle = useAnimatedStyle(() => ({
    height: height.value,
    borderRadius: height.value / 2,
  }));

  const expand = () => {
    height.value = withTiming(TRACK_BAR_HEIGHT_PRESSED, {
      duration: TRACK_BAR_ANIM_MS,
    });
  };

  const collapse = () => {
    height.value = withTiming(TRACK_BAR_HEIGHT, {
      duration: TRACK_BAR_ANIM_MS,
    });
  };

  return { animatedStyle, expand, collapse };
}

const AudioControls: React.FC = () => {
  const {
    isReady,
    play,
    pause,
    seekToTime,
    playbackState,
    muted,
    setMuted,
    currentTime,
    duration,
  } = useAudioTagContext();

  const [progressWidth, setProgressWidth] = useState(0);
  const [scrubTime, setScrubTime] = useState<number | null>(null);

  const progressTrackAnim = useExpandableTrackHeight();

  const progressTrackRef = useRef<View>(null);
  const progressMetricsRef = useRef({ left: 0, width: 0 });
  const progressWidthRef = useRef(0);
  const durationRef = useRef(duration);
  const wasPlayingBeforeScrubRef = useRef(false);
  durationRef.current = duration;
  progressWidthRef.current = progressWidth;

  const onStart = useCallback(
    (x: number) => {
      progressTrackAnim.expand();
      const d = durationRef.current;
      if (playbackState === 'playing') {
        wasPlayingBeforeScrubRef.current = true;
        pause();
      } else {
        wasPlayingBeforeScrubRef.current = false;
      }
      progressTrackRef.current?.measureInWindow((left, _y, width, _h) => {
        progressMetricsRef.current.left = left;
        progressMetricsRef.current.width = width;
        setScrubTime(
          timeFromLocationX(x, width || progressWidthRef.current, d)
        );
      });
    },
    [playbackState, pause, progressTrackAnim]
  );

  const onUpdate = useCallback((x: number) => {
    const d = durationRef.current;
    const w = progressMetricsRef.current.width || progressWidthRef.current;
    setScrubTime(timeFromLocationX(x, w, d));
  }, []);

  const seekTo = useCallback(
    (x: number) => {
      const d = durationRef.current;
      const w = progressMetricsRef.current.width || progressWidthRef.current;
      const t = timeFromLocationX(x, w, d);
      seekToTime(t);
    },
    [seekToTime]
  );

  const onEnd = useCallback(
    (x: number, flag: boolean = true) => {
      if (flag) {
        seekTo(x);
      }
      progressTrackAnim.collapse();
      setScrubTime(null);
      if (wasPlayingBeforeScrubRef.current) {
        setTimeout(() => {
          play();
        }, NATIVE_DELAY_MS);
      }
    },
    [play, progressTrackAnim, seekTo]
  );

  const onCancel = useCallback(() => {
    progressTrackAnim.collapse();
    setScrubTime(null);
  }, [progressTrackAnim]);

  const onTapSeek = useCallback(
    (x: number) => {
      if (playbackState === 'playing') {
        wasPlayingBeforeScrubRef.current = true;
        pause();
      } else {
        wasPlayingBeforeScrubRef.current = false;
      }
      setTimeout(() => {
        seekTo(x);
        setTimeout(() => {
          onEnd(x, false);
        }, NATIVE_DELAY_MS);
      }, NATIVE_DELAY_MS);
    },
    [seekTo, onEnd, playbackState, pause]
  );

  const scrubGesture = useMemo(() => {
    const panGesture = Gesture.Pan()
      .minDistance(SCRUB_PAN_MIN_DISTANCE)
      .onStart((e) => {
        scheduleOnRN(onStart, e.x);
      })
      .onUpdate((e) => {
        scheduleOnRN(onUpdate, e.x);
      })
      .onEnd((e) => {
        scheduleOnRN(onEnd, e.x);
      })
      .onFinalize((_e, success) => {
        if (!success) {
          scheduleOnRN(onCancel);
        }
      });

    const tapGesture = Gesture.Tap()
      .maxDistance(14)
      .onEnd((e, success) => {
        if (success) {
          scheduleOnRN(onTapSeek, e.x);
        }
      });

    return Gesture.Race(panGesture, tapGesture);
  }, [onStart, onUpdate, onEnd, onCancel, onTapSeek]);

  const onPlayPausePress = () => {
    if (playbackState === 'playing') {
      pause();
    } else {
      play();
    }
  };

  const onProgressTrackLayout = (e: LayoutChangeEvent) => {
    const w = e.nativeEvent.layout.width;
    setProgressWidth(w);
    progressWidthRef.current = w;
    progressTrackRef.current?.measureInWindow((left, _y, width, _h) => {
      progressMetricsRef.current.left = left;
      progressMetricsRef.current.width = width;
    });
  };

  if (!isReady) {
    return (
      <View style={styles.container}>
        <View style={styles.loadingRow}>
          <ActivityIndicator color="#333" size="small" />
          <Text style={[styles.loadingText, { marginLeft: 8 }]}>Loading…</Text>
        </View>
      </View>
    );
  }

  const displayTime = scrubTime ?? currentTime;
  const progress = duration > 0 ? displayTime / duration : 0;

  return (
    <View style={styles.container}>
      <GestureHandlerRootView style={styles.topRow}>
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
      </GestureHandlerRootView>
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
