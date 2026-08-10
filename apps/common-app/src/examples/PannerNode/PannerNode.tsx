import React, { useEffect, useRef, useState, FC } from 'react';
import { Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';
import {
  AudioContext,
  GainNode,
  OscillatorNode,
  PannerNode,
} from 'react-native-audio-api';
import type { DistanceModelType } from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors, layout } from '../../styles';

const LABEL_WIDTH = 100;
const DISTANCE_MODELS: DistanceModelType[] = [
  'inverse',
  'linear',
  'exponential',
];

type SpatialState = {
  positionX: number;
  positionY: number;
  positionZ: number;
  orientationX: number;
  orientationY: number;
  orientationZ: number;
};

const INITIAL_SPATIAL: SpatialState = {
  positionX: 0,
  positionY: 0,
  positionZ: -1,
  orientationX: 0,
  orientationY: 0,
  orientationZ: 1,
};

type Preset = {
  label: string;
  spatial: SpatialState;
  coneEnabled?: boolean;
  hint: string;
};

const PRESETS: Preset[] = [
  {
    label: 'Center',
    spatial: { ...INITIAL_SPATIAL },
    coneEnabled: false,
    hint: 'In front of listener (cone off) — move Pos X for L/R pan',
  },
  {
    label: 'Left',
    spatial: { ...INITIAL_SPATIAL, positionX: -3 },
    coneEnabled: false,
    hint: 'Stronger in the left ear (cone off)',
  },
  {
    label: 'Right',
    spatial: { ...INITIAL_SPATIAL, positionX: 3 },
    coneEnabled: false,
    hint: 'Stronger in the right ear (cone off)',
  },
  {
    label: 'Far',
    spatial: { ...INITIAL_SPATIAL, positionZ: -8 },
    coneEnabled: false,
    hint: 'Quieter via distance attenuation (cone off)',
  },
  {
    label: 'Away',
    spatial: {
      ...INITIAL_SPATIAL,
      orientationX: 0,
      orientationY: 0,
      orientationZ: -1,
    },
    coneEnabled: true,
    hint: 'Cone on + facing away — should be much quieter / silent',
  },
];

const PannerNodeExample: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [gain, setGain] = useState(0.4);
  const [spatial, setSpatial] = useState<SpatialState>(INITIAL_SPATIAL);
  // Cone off by default: with a narrow cone, moving Pos X quickly points the
  // source away from the listener and hits coneOuterGain (silence) — that
  // looked like a panning bug at X≈±1.
  const [coneEnabled, setConeEnabled] = useState(false);
  const [distanceModel, setDistanceModel] =
    useState<DistanceModelType>('inverse');
  const [rolloffFactor, setRolloffFactor] = useState(1);
  const [hint, setHint] = useState(
    'Use headphones. Cone OFF: Pos X pans L/R, Pos Z is distance. Turn Cone ON only to test orientation.'
  );

  const audioContextRef = useRef<AudioContext | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);
  const pannerRef = useRef<PannerNode | null>(null);

  useEffect(() => {
    audioContextRef.current = new AudioContext();
    return () => {
      audioContextRef.current?.close();
      audioContextRef.current = null;
    };
  }, []);

  const applyCone = (panner: PannerNode, enabled: boolean) => {
    if (enabled) {
      panner.coneInnerAngle = 60;
      panner.coneOuterAngle = 90;
      panner.coneOuterGain = 0;
    } else {
      panner.coneInnerAngle = 360;
      panner.coneOuterAngle = 360;
      panner.coneOuterGain = 0;
    }
  };

  const applySpatial = (next: SpatialState) => {
    const panner = pannerRef.current;
    if (!panner) {
      return;
    }
    panner.positionX.value = next.positionX;
    panner.positionY.value = next.positionY;
    panner.positionZ.value = next.positionZ;
    panner.orientationX.value = next.orientationX;
    panner.orientationY.value = next.orientationY;
    panner.orientationZ.value = next.orientationZ;
  };

  const updateSpatial = <K extends keyof SpatialState>(
    key: K,
    value: SpatialState[K]
  ) => {
    setSpatial((prev) => {
      const next = { ...prev, [key]: value };
      applySpatial(next);
      return next;
    });
  };

  const setup = () => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    const ctx = audioContextRef.current;
    oscillatorRef.current = ctx.createOscillator();
    oscillatorRef.current.type = 'sine';
    oscillatorRef.current.frequency.value = 440;

    gainRef.current = ctx.createGain();
    gainRef.current.gain.value = gain;

    pannerRef.current = ctx.createPanner();
    pannerRef.current.distanceModel = distanceModel;
    pannerRef.current.rolloffFactor = rolloffFactor;
    applyCone(pannerRef.current, coneEnabled);
    applySpatial(spatial);

    oscillatorRef.current.connect(gainRef.current);
    gainRef.current.connect(pannerRef.current);
    pannerRef.current.connect(ctx.destination);
  };

  const handlePlayPause = () => {
    if (isPlaying) {
      oscillatorRef.current?.stop(0);
      oscillatorRef.current = null;
      gainRef.current = null;
      pannerRef.current = null;
    } else {
      setup();
      oscillatorRef.current?.start(0);
    }
    setIsPlaying((prev) => !prev);
  };

  const applyPreset = (preset: Preset) => {
    setSpatial(preset.spatial);
    applySpatial(preset.spatial);
    if (preset.coneEnabled !== undefined) {
      setConeEnabled(preset.coneEnabled);
      if (pannerRef.current) {
        applyCone(pannerRef.current, preset.coneEnabled);
      }
    }
    setHint(preset.hint);
  };

  return (
    <Container>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        <Text style={styles.title}>PannerNode</Text>
        <Text style={styles.hint}>{hint}</Text>
        <Spacer.Vertical size={16} />

        <Button
          onPress={handlePlayPause}
          title={isPlaying ? 'Stop' : 'Play tone'}
        />
        <Spacer.Vertical size={20} />

        <Text style={styles.section}>Presets</Text>
        <View style={styles.row}>
          {PRESETS.map((preset) => (
            <Pressable
              key={preset.label}
              style={styles.chip}
              onPress={() => applyPreset(preset)}>
              <Text style={styles.chipText}>{preset.label}</Text>
            </Pressable>
          ))}
        </View>
        <Spacer.Vertical size={16} />

        <Pressable
          style={[styles.chip, coneEnabled && styles.chipActive]}
          onPress={() => {
            const next = !coneEnabled;
            setConeEnabled(next);
            if (pannerRef.current) {
              applyCone(pannerRef.current, next);
            }
            setHint(
              next
                ? 'Cone on — orientation affects loudness'
                : 'Cone off (360°) — orientation has no effect'
            );
          }}>
          <Text style={styles.chipText}>
            Cone {coneEnabled ? 'ON' : 'OFF'}
          </Text>
        </Pressable>
        <Spacer.Vertical size={12} />

        <Text style={styles.section}>Distance model</Text>
        <View style={styles.row}>
          {DISTANCE_MODELS.map((model) => (
            <Pressable
              key={model}
              style={[
                styles.chip,
                distanceModel === model && styles.chipActive,
              ]}
              onPress={() => {
                setDistanceModel(model);
                if (pannerRef.current) {
                  pannerRef.current.distanceModel = model;
                }
              }}>
              <Text style={styles.chipText}>{model}</Text>
            </Pressable>
          ))}
        </View>
        <Spacer.Vertical size={16} />

        <Slider
          label="Gain"
          value={gain}
          onValueChange={(value) => {
            setGain(value);
            if (gainRef.current) {
              gainRef.current.gain.value = value;
            }
          }}
          min={0}
          max={1}
          step={0.01}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={12} />
        <Slider
          label="Rolloff"
          value={rolloffFactor}
          onValueChange={(value) => {
            setRolloffFactor(value);
            if (pannerRef.current) {
              pannerRef.current.rolloffFactor = value;
            }
          }}
          min={0}
          max={4}
          step={0.1}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={16} />

        <Text style={styles.section}>Position</Text>
        <Slider
          label="Pos X"
          value={spatial.positionX}
          onValueChange={(value) => updateSpatial('positionX', value)}
          min={-8}
          max={8}
          step={0.1}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={10} />
        <Slider
          label="Pos Y"
          value={spatial.positionY}
          onValueChange={(value) => updateSpatial('positionY', value)}
          min={-8}
          max={8}
          step={0.1}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={10} />
        <Slider
          label="Pos Z"
          value={spatial.positionZ}
          onValueChange={(value) => updateSpatial('positionZ', value)}
          min={-12}
          max={4}
          step={0.1}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={16} />

        <Text style={styles.section}>Orientation</Text>
        <Slider
          label="Ori X"
          value={spatial.orientationX}
          onValueChange={(value) => updateSpatial('orientationX', value)}
          min={-1}
          max={1}
          step={0.05}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={10} />
        <Slider
          label="Ori Y"
          value={spatial.orientationY}
          onValueChange={(value) => updateSpatial('orientationY', value)}
          min={-1}
          max={1}
          step={0.05}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={10} />
        <Slider
          label="Ori Z"
          value={spatial.orientationZ}
          onValueChange={(value) => updateSpatial('orientationZ', value)}
          min={-1}
          max={1}
          step={0.05}
          minLabelWidth={LABEL_WIDTH}
        />
        <Spacer.Vertical size={24} />
      </ScrollView>
    </Container>
  );
};

const styles = StyleSheet.create({
  scrollContent: {
    paddingVertical: layout.spacing,
    paddingBottom: 40,
  },
  title: {
    color: colors.white,
    fontSize: 22,
    fontWeight: '700',
    marginBottom: 8,
  },
  hint: {
    color: colors.white,
    opacity: 0.7,
    fontSize: 14,
    lineHeight: 20,
  },
  section: {
    color: colors.white,
    fontSize: 14,
    fontWeight: '600',
    marginBottom: 10,
    opacity: 0.85,
  },
  row: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 8,
  },
  chip: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: layout.radius,
    paddingHorizontal: 12,
    paddingVertical: 8,
    marginRight: 8,
    marginBottom: 8,
  },
  chipActive: {
    backgroundColor: colors.main,
    borderColor: colors.main,
  },
  chipText: {
    color: colors.white,
    textTransform: 'capitalize',
  },
});

export default PannerNodeExample;
