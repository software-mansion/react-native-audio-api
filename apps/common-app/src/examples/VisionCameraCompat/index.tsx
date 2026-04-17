import type { FC } from 'react';
import React, { useEffect, useRef, useState } from 'react';
import {
  ActivityIndicator,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import type {
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { AudioContext, AudioManager } from 'react-native-audio-api';
import {
  Camera,
  useCameraDevice,
  useCameraPermission,
} from 'react-native-vision-camera';

import { Button, Container, Spacer } from '../../components';
import { colors, layout } from '../../styles';
import sampleAsset from '../AudioFile/voice-sample-landing.mp3';

type CameraStatus = 'requesting' | 'granted' | 'denied';

const VisionCameraCompat: FC = () => {
  const device = useCameraDevice('back');
  const { hasPermission, requestPermission } = useCameraPermission();

  const audioContextRef = useRef<AudioContext | null>(new AudioContext());
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const sourceNodeRef = useRef<AudioBufferSourceNode | null>(null);
  const hasRequestedPermissionRef = useRef(false);
  const isMountedRef = useRef(true);

  const [cameraStatus, setCameraStatus] = useState<CameraStatus>(
    hasPermission ? 'granted' : 'requesting'
  );
  const [isBufferLoading, setIsBufferLoading] = useState(true);
  const [audioError, setAudioError] = useState<string | null>(null);
  const [isPlaying, setIsPlaying] = useState(false);

  const stopPlayback = async (deactivateSession: boolean = true) => {
    const sourceNode = sourceNodeRef.current;
    sourceNodeRef.current = null;

    if (sourceNode) {
      sourceNode.onEnded = null;
      try {
        sourceNode.stop(audioContextRef.current?.currentTime ?? 0);
      } catch (error: unknown) {
        console.warn('Failed to stop audio source cleanly:', error);
      }
    }

    if (isMountedRef.current) {
      setIsPlaying(false);
    }

    if (deactivateSession) {
      try {
        await AudioManager.setAudioSessionActivity(false);
      } catch (error: unknown) {
        console.warn('Failed to deactivate audio session:', error);
      }

      try {
        await audioContextRef.current?.suspend();
      } catch (error: unknown) {
        console.warn('Failed to suspend audio context:', error);
      }
    }
  };

  useEffect(() => {
    if (hasPermission) {
      setCameraStatus('granted');
    }
  }, [hasPermission]);

  useEffect(() => {
    if (hasPermission || hasRequestedPermissionRef.current) {
      return;
    }

    hasRequestedPermissionRef.current = true;
    setCameraStatus('requesting');

    let cancelled = false;

    const ensurePermission = async () => {
      try {
        const granted = await requestPermission();

        if (!cancelled && isMountedRef.current) {
          setCameraStatus(granted ? 'granted' : 'denied');
        }
      } catch (error: unknown) {
        if (!cancelled && isMountedRef.current) {
          setCameraStatus('denied');
        }
        console.warn('Failed to request camera permission:', error);
      }
    };

    ensurePermission().catch((error: unknown) => {
      if (!cancelled && isMountedRef.current) {
        setCameraStatus('denied');
      }
      console.warn('Camera permission request failed:', error);
    });

    return () => {
      cancelled = true;
    };
  }, [hasPermission, requestPermission]);

  useEffect(() => {
    let cancelled = false;

    const loadAudioBuffer = async () => {
      try {
        const context = audioContextRef.current ?? new AudioContext();
        audioContextRef.current = context;

        const buffer = await context.decodeAudioData(sampleAsset);

        if (!cancelled && isMountedRef.current) {
          audioBufferRef.current = buffer;
          setAudioError(null);
        }
      } catch (error: unknown) {
        if (!cancelled && isMountedRef.current) {
          setAudioError(
            error instanceof Error
              ? error.message
              : 'Failed to decode the bundled audio sample.'
          );
        }
      } finally {
        if (!cancelled && isMountedRef.current) {
          setIsBufferLoading(false);
        }
      }
    };

    loadAudioBuffer().catch((error: unknown) => {
      if (!cancelled && isMountedRef.current) {
        setAudioError(
          error instanceof Error
            ? error.message
            : 'Failed to decode the bundled audio sample.'
        );
        setIsBufferLoading(false);
      }
    });

    return () => {
      cancelled = true;
    };
  }, []);

  useEffect(() => {
    return () => {
      isMountedRef.current = false;
      audioBufferRef.current = null;

      const sourceNode = sourceNodeRef.current;
      sourceNodeRef.current = null;
      if (sourceNode) {
        sourceNode.onEnded = null;
        try {
          sourceNode.stop(audioContextRef.current?.currentTime ?? 0);
        } catch (error: unknown) {
          console.warn('Failed to stop source during cleanup:', error);
        }
      }

      const context = audioContextRef.current;
      audioContextRef.current = null;
      if (context) {
        context.suspend().catch((error: unknown) => {
          console.warn('Failed to suspend audio context during cleanup:', error);
        });
      }

      AudioManager.setAudioSessionActivity(false).catch((error: unknown) => {
        console.warn('Failed to deactivate audio session during cleanup:', error);
      });
    };
  }, []);

  const togglePlayback = async () => {
    if (isPlaying) {
      await stopPlayback();
      return;
    }

    const context = audioContextRef.current;
    const buffer = audioBufferRef.current;

    if (!context || !buffer) {
      return;
    }

    setAudioError(null);

    try {
      AudioManager.setAudioSessionOptions({
        iosCategory: 'playback',
        iosMode: 'default',
        iosOptions: [],
      });

      const active = await AudioManager.setAudioSessionActivity(true);
      if (!active) {
        setAudioError('Failed to activate the audio session.');
        return;
      }

      if (context.state === 'suspended') {
        await context.resume();
      }

      const sourceNode = context.createBufferSource({ pitchCorrection: true });
      sourceNode.buffer = buffer;
      sourceNode.onEnded = () => {
        sourceNodeRef.current = null;
        if (isMountedRef.current) {
          setIsPlaying(false);
        }
        AudioManager.setAudioSessionActivity(false).catch(() => {});
      };
      sourceNode.connect(context.destination);
      sourceNode.start(context.currentTime);

      sourceNodeRef.current = sourceNode;
      setIsPlaying(true);
    } catch (error: unknown) {
      setAudioError(
        error instanceof Error
          ? error.message
          : 'Failed to start audio playback.'
      );
      await stopPlayback();
    }
  };

  const cameraMessage =
    cameraStatus === 'requesting'
      ? 'Requesting camera permission...'
      : !hasPermission
        ? 'Camera permission denied. Enable it in settings to see the preview.'
        : device == null
          ? 'No back camera device is available on this simulator or device.'
          : 'Back camera preview is active.';

  const audioMessage = audioError
    ? audioError
    : isBufferLoading
      ? 'Loading bundled audio sample...'
      : isPlaying
        ? 'Playing bundled audio sample.'
        : 'Bundled audio sample is ready.';

  return (
    <Container headless disablePadding style={styles.container}>
      <View style={styles.previewShell}>
        {hasPermission && device != null ? (
          <Camera
            device={device}
            isActive
            style={StyleSheet.absoluteFill}
          />
        ) : (
          <View style={styles.previewFallback}>
            {cameraStatus === 'requesting' && (
              <ActivityIndicator color={colors.white} />
            )}
            <Text style={styles.previewFallbackText}>{cameraMessage}</Text>
          </View>
        )}
      </View>

      <View style={styles.panel}>
        <Text style={styles.title}>Vision Camera Compatibility</Text>
        <Text style={styles.statusText}>{cameraMessage}</Text>
        <Spacer.Vertical size={8} />
        <Text style={styles.statusText}>{audioMessage}</Text>
        <Spacer.Vertical size={16} />
        <Button
          title={isPlaying ? 'Stop' : 'Play'}
          onPress={() => {
            togglePlayback().catch((error: unknown) => {
              console.warn('Playback toggle failed:', error);
            });
          }}
          disabled={isBufferLoading || audioBufferRef.current == null}
          width={140}
        />
      </View>
    </Container>
  );
};

export default VisionCameraCompat;

const styles = StyleSheet.create({
  container: {
    paddingHorizontal: 16,
    paddingBottom: 16,
  },
  previewShell: {
    flex: 1,
    width: '100%',
    overflow: 'hidden',
    borderRadius: 16,
    borderWidth: 1,
    borderColor: colors.border,
    backgroundColor: colors.backgroundDark,
    minHeight: 320,
  },
  previewFallback: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    paddingHorizontal: 24,
    gap: layout.spacing * 2,
  },
  previewFallbackText: {
    color: colors.white,
    textAlign: 'center',
    opacity: 0.8,
    lineHeight: 20,
  },
  panel: {
    paddingTop: 16,
    alignItems: 'center',
  },
  title: {
    color: colors.white,
    fontSize: 20,
    fontWeight: '700',
    textAlign: 'center',
  },
  statusText: {
    color: colors.white,
    textAlign: 'center',
    opacity: 0.8,
    lineHeight: 20,
  },
});
