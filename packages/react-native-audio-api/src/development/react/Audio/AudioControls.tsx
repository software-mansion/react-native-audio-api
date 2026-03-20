import { Pause, Play, Volume, VolumeX } from 'lucide-react-native';
import React, { useCallback, useEffect, useMemo, useState } from 'react';
import {
  ActivityIndicator,
  LayoutChangeEvent,
  PanResponder,
  Platform,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import { useAudioTagContext } from './AudioTagContext';

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
    flexShrink: 0,
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
    height: 6,
    justifyContent: 'center',
    marginRight: 10,
  },
  progressTrackInner: {
    flex: 1,
    height: 6,
    backgroundColor: '#ccc',
    borderRadius: 3,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: '#000',
    borderRadius: 3,
  },
  volumeIcon: {
    padding: 4,
    flexShrink: 0,
  },
  bottomRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    marginTop: 8,
  },
  volumeTrack: {
    width: '50%',
    height: 10,
    flexDirection: 'column',
    justifyContent: 'center',
  },
  volumeTrackPan: {
    flex: 1,
    width: '100%',
    justifyContent: 'center',
  },
  volumeTrackInner: {
    flex: 1,
    height: 6,
    backgroundColor: '#ccc',
    borderRadius: 3,
    overflow: 'hidden',
  },
  volumeFill: {
    height: '100%',
    backgroundColor: '#000',
    borderRadius: 3,
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

const AudioControls: React.FC = () => {
  const {
    isReady,
    play,
    pause,
    playbackState,
    volume,
    setVolume,
    muted,
    setMuted,
    currentTime,
    duration,
  } = useAudioTagContext();

  const [trackWidth, setTrackWidth] = useState(0);

  const updateVolumeFromPosition = useCallback(
    (locationX: number) => {
      if (trackWidth <= 0) return;
      const pct = Math.max(0, Math.min(1, locationX / trackWidth));
      setVolume(pct);
    },
    [trackWidth, setVolume]
  );

  const panResponder = useMemo(
    () =>
      PanResponder.create({
        onStartShouldSetPanResponder: () => true,
        onMoveShouldSetPanResponder: () => true,
        onPanResponderGrant: (e) =>
          updateVolumeFromPosition(e.nativeEvent.locationX),
        onPanResponderMove: (e) =>
          updateVolumeFromPosition(e.nativeEvent.locationX),
      }),
    [updateVolumeFromPosition]
  );

  const onPlayPausePress = useCallback(() => {
    if (playbackState === 'playing') {
      pause();
    } else {
      play();
    }
  }, [playbackState, pause, play]);

  const onVolumeTrackLayout = useCallback((e: LayoutChangeEvent) => {
    setTrackWidth(e.nativeEvent.layout.width);
  }, []);

  useEffect(() => {
    console.log('playbackState', playbackState);
  }, [playbackState]);

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

  const progress = duration > 0 ? currentTime / duration : 0;

  return (
    <View style={styles.container}>
      <View style={styles.topRow}>
        <Pressable style={styles.playPause} onPress={onPlayPausePress}>
          {playbackState === 'playing' ? (
            <Pause color="#000" size={20} strokeWidth={2.5} />
          ) : (
            <Play color="#000" size={20} strokeWidth={2.5} />
          )}
        </Pressable>

        <Text style={styles.timeText}>
          {formatTime(currentTime)} / {formatTime(duration)}
        </Text>

        <View style={styles.progressTrack}>
          <View style={styles.progressTrackInner}>
            <View
              style={[
                styles.progressFill,
                { width: `${Math.min(1, progress) * 100}%` },
              ]}
            />
          </View>
        </View>

        <Pressable style={styles.volumeIcon} onPress={() => setMuted(!muted)}>
          {muted ? (
            <VolumeX color="#000" size={20} strokeWidth={2.5} />
          ) : (
            <Volume color="#000" size={20} strokeWidth={2.5} />
          )}
        </Pressable>
      </View>

      <View style={styles.bottomRow}>
        <View style={styles.volumeTrack} onLayout={onVolumeTrackLayout}>
          <View {...panResponder.panHandlers} style={styles.volumeTrackPan}>
            <View style={styles.volumeTrackInner}>
              <View
                style={[
                  styles.volumeFill,
                  { width: `${(muted ? 0 : volume) * 100}%` },
                ]}
              />
            </View>
          </View>
        </View>
      </View>
    </View>
  );
};

export default AudioControls;
