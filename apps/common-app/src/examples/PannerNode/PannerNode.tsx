import React, { useEffect, useRef, useState, FC } from 'react';
import { Pressable, ScrollView, StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  PannerNode,
} from 'react-native-audio-api';

import { Button, Container, Slider, Spacer } from '../../components';
import { colors, layout } from '../../styles';

const MUSIC_URL =
  'https://docs.swmansion.com/react-native-audio-api/audio/music/example-music-02.mp3';

const ORBIT_RADIUS = 2;
const VIEW_SIZE = 220;
const VIEW_SCALE = VIEW_SIZE / 2 / 4.5;
const SPEAKER_SIZE = 32;

type Mode = 'figureEight' | 'guided';

type Vec3 = { x: number; y: number; z: number };

type Pose = {
  position: Vec3;
  /** Defaults to facing the listener, which keeps the cone fully open. */
  orientation?: Vec3;
};

type GuidedStep = {
  title: string;
  expect: string;
  durationSeconds: number;
  pose: (progress: number) => Pose;
};

const at = (x: number, y: number, z: number): Pose => ({
  position: { x, y, z },
});

const facingAway = (angle: number): Pose => ({
  position: { x: 0, y: 0, z: -ORBIT_RADIUS },
  orientation: { x: Math.sin(angle), y: 0, z: Math.cos(angle) },
});

const GUIDED_STEPS: GuidedStep[] = [
  {
    title: 'In front',
    expect: 'Equally loud in both ears',
    durationSeconds: 4,
    pose: () => at(0, 0, -ORBIT_RADIUS),
  },
  {
    title: 'Left',
    expect: 'Only in the left ear',
    durationSeconds: 4,
    pose: () => at(-ORBIT_RADIUS, 0, 0),
  },
  {
    title: 'Right',
    expect: 'Only in the right ear',
    durationSeconds: 4,
    pose: () => at(ORBIT_RADIUS, 0, 0),
  },
  {
    title: 'Sweep',
    expect: 'Glides smoothly from the left ear to the right ear',
    durationSeconds: 6,
    pose: (p) => {
      const angle = Math.PI * (p - 0.5);
      return at(
        ORBIT_RADIUS * Math.sin(angle),
        0,
        -ORBIT_RADIUS * Math.cos(angle)
      );
    },
  },
  {
    title: 'Fly away',
    expect: 'Fades out while staying centred',
    durationSeconds: 5,
    pose: (p) => at(0, 0, -ORBIT_RADIUS - 14 * p),
  },
  {
    title: 'Come back',
    expect: 'Fades back in while staying centred',
    durationSeconds: 5,
    pose: (p) => at(0, 0, -ORBIT_RADIUS - 14 * (1 - p)),
  },
  {
    title: 'Turn away',
    expect: 'Gets quieter as the speaker turns its back to you',
    durationSeconds: 5,
    pose: (p) => facingAway(Math.PI * p),
  },
  {
    title: 'Turn back',
    expect: 'Gets louder as the speaker faces you again',
    durationSeconds: 5,
    pose: (p) => facingAway(Math.PI * (1 - p)),
  },
];

const FIGURE_EIGHT_CENTRE_Z = -2.5;

/** A figure 8 lying in front of the head, one loop on each side. */
const figureEightPose = (angle: number): Pose =>
  at(
    2.5 * Math.sin(angle),
    0,
    FIGURE_EIGHT_CENTRE_Z + 1.5 * Math.sin(2 * angle)
  );

const figureEightOutline = Array.from({ length: 48 }, (_, i) => {
  const { x, z } = figureEightPose((2 * Math.PI * i) / 48).position;
  return { x, z };
});

/** Degrees in the horizontal plane: 0 in front, -90 left, 90 right, ±180 behind. */
const azimuthOf = ({ x, z }: Vec3) => (Math.atan2(x, -z) * 180) / Math.PI;

/**
 * Equal-power panning folds the back half onto the front, so behind sounds
 * centred.
 */
const expectedBalance = (position: Vec3) => {
  const azimuth = azimuthOf(position);
  const folded =
    azimuth > 90 ? 180 - azimuth : azimuth < -90 ? -180 - azimuth : azimuth;
  return folded / 90;
};

const describeFigureEightPosition = (position: Vec3) => {
  const azimuth = azimuthOf(position);
  const loudness = position.z > FIGURE_EIGHT_CENTRE_Z ? 'louder' : 'quieter';
  if (Math.abs(azimuth) < 15) {
    return 'Crossing the middle: centred';
  }
  return `${azimuth < 0 ? 'Left' : 'Right'} side, getting ${loudness}`;
};

const facingOf = ({ position, orientation }: Pose): Vec3 =>
  orientation ?? { x: -position.x, y: -position.y, z: -position.z };

/** 0 when the speaker points straight at the listener, 180 when away. */
const angleFromListener = (pose: Pose) => {
  const facing = facingOf(pose);
  const { position } = pose;
  const cosine =
    -(facing.x * position.x + facing.y * position.y + facing.z * position.z) /
    (Math.hypot(facing.x, facing.y, facing.z) *
      Math.hypot(position.x, position.y, position.z));
  return (Math.acos(Math.min(Math.max(cosine, -1), 1)) * 180) / Math.PI;
};

const describeFacing = (pose: Pose) => {
  const angle = angleFromListener(pose);
  if (angle < 10) {
    return 'Speaker points at you';
  }
  if (angle > 170) {
    return 'Speaker points away from you';
  }
  return `Speaker turned ${Math.round(angle)}° away from you`;
};

const applyPose = (panner: PannerNode, pose: Pose) => {
  const { position } = pose;
  const facing = facingOf(pose);
  panner.positionX.value = position.x;
  panner.positionY.value = position.y;
  panner.positionZ.value = position.z;
  panner.orientationX.value = facing.x;
  panner.orientationY.value = facing.y;
  panner.orientationZ.value = facing.z;
};

const BalanceMeter: FC<{ balance: number }> = ({ balance }) => (
  <View>
    <View style={styles.meterLabels}>
      <Text style={styles.meterLabel}>L</Text>
      <Text style={styles.meterLabel}>centre</Text>
      <Text style={styles.meterLabel}>R</Text>
    </View>
    <View style={styles.meterTrack}>
      <View style={styles.meterCentre} />
      <View
        style={[styles.meterMarker, { left: `${((balance + 1) / 2) * 100}%` }]}
      />
    </View>
  </View>
);

const toViewPoint = ({ x, z }: { x: number; z: number }) => {
  const clamp = (value: number) => Math.min(Math.max(value, 0), VIEW_SIZE);
  return {
    left: clamp(VIEW_SIZE / 2 + x * VIEW_SCALE),
    top: clamp(VIEW_SIZE / 2 + z * VIEW_SCALE),
  };
};

const TopDownView: FC<{ pose: Pose; showPath: boolean }> = ({
  pose,
  showPath,
}) => {
  const facing = facingOf(pose);
  const facingDegrees = (Math.atan2(facing.x, -facing.z) * 180) / Math.PI;
  const { left, top } = toViewPoint(pose.position);
  return (
    <View style={styles.view}>
      {showPath &&
        figureEightOutline.map((point, i) => {
          const pathPoint = toViewPoint(point);
          return (
            <View
              key={i}
              style={[
                styles.pathDot,
                { left: pathPoint.left - 1, top: pathPoint.top - 1 },
              ]}
            />
          );
        })}
      <View style={styles.head}>
        <View style={styles.nose} />
        <Text style={styles.earLeft}>L</Text>
        <Text style={styles.earRight}>R</Text>
      </View>
      <View
        style={[
          styles.speaker,
          {
            left: left - SPEAKER_SIZE / 2,
            top: top - SPEAKER_SIZE / 2,
            transform: [{ rotate: `${facingDegrees}deg` }],
          },
        ]}
      >
        <View style={styles.speakerPointer} />
        <View style={styles.dot} />
      </View>
      <Text style={styles.viewCaption}>
        top-down, front is up · arrow = where the speaker points
      </Text>
    </View>
  );
};

const PannerNodeExample: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [mode, setMode] = useState<Mode>('figureEight');
  const [periodSeconds, setPeriodSeconds] = useState(8);
  const [pose, setPose] = useState<Pose>(at(0, 0, -ORBIT_RADIUS));
  const [stepIndex, setStepIndex] = useState(0);
  const [stepProgress, setStepProgress] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const musicBufferRef = useRef<AudioBuffer | null>(null);
  const sourceRef = useRef<AudioBufferSourceNode | null>(null);
  const pannerRef = useRef<PannerNode | null>(null);
  const frameRef = useRef<number | null>(null);

  const settingsRef = useRef({ mode, periodSeconds });
  settingsRef.current = { mode, periodSeconds };
  const pathAngleRef = useRef(0);
  const guidedRef = useRef({ index: 0, startTime: 0 });
  const lastTimeRef = useRef(0);

  useEffect(() => {
    const ctx = new AudioContext();
    audioContextRef.current = ctx;
    fetch(MUSIC_URL)
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer))
      .then((buffer) => {
        musicBufferRef.current = buffer;
      })
      .catch((error) => {
        console.error('PannerNode: failed to load music', error);
      })
      .finally(() => setIsLoading(false));
    return () => {
      if (frameRef.current !== null) {
        cancelAnimationFrame(frameRef.current);
      }
      audioContextRef.current?.close();
      audioContextRef.current = null;
    };
  }, []);

  const goToStep = (index: number) => {
    const ctx = audioContextRef.current;
    const wrapped = (index + GUIDED_STEPS.length) % GUIDED_STEPS.length;
    guidedRef.current = { index: wrapped, startTime: ctx?.currentTime ?? 0 };
    setStepIndex(wrapped);
    setStepProgress(0);
  };

  const tick = () => {
    const ctx = audioContextRef.current;
    const panner = pannerRef.current;
    if (!ctx || !panner) {
      return;
    }
    const now = ctx.currentTime;
    const elapsed = now - lastTimeRef.current;
    lastTimeRef.current = now;

    const settings = settingsRef.current;
    let nextPose: Pose;
    if (settings.mode === 'figureEight') {
      pathAngleRef.current += (2 * Math.PI * elapsed) / settings.periodSeconds;
      nextPose = figureEightPose(pathAngleRef.current);
    } else {
      let step = GUIDED_STEPS[guidedRef.current.index];
      if (now - guidedRef.current.startTime >= step.durationSeconds) {
        goToStep(guidedRef.current.index + 1);
        step = GUIDED_STEPS[guidedRef.current.index];
      }
      const progress = Math.min(
        (now - guidedRef.current.startTime) / step.durationSeconds,
        1
      );
      setStepProgress(progress);
      nextPose = step.pose(progress);
    }

    applyPose(panner, nextPose);
    setPose(nextPose);
    frameRef.current = requestAnimationFrame(tick);
  };

  const start = () => {
    const ctx = audioContextRef.current;
    if (!ctx || !musicBufferRef.current) {
      return;
    }
    const source = ctx.createBufferSource();
    source.buffer = musicBufferRef.current;
    source.loop = true;

    const panner = ctx.createPanner();
    panner.distanceModel = 'inverse';
    panner.refDistance = 1;
    panner.rolloffFactor = 1;
    panner.coneInnerAngle = 60;
    panner.coneOuterAngle = 180;
    panner.coneOuterGain = 0.1;

    source.connect(panner);
    panner.connect(ctx.destination);
    sourceRef.current = source;
    pannerRef.current = panner;

    lastTimeRef.current = ctx.currentTime;
    goToStep(0);
    source.start();
    frameRef.current = requestAnimationFrame(tick);
  };

  const stop = () => {
    if (frameRef.current !== null) {
      cancelAnimationFrame(frameRef.current);
      frameRef.current = null;
    }
    sourceRef.current?.stop();
    sourceRef.current = null;
    pannerRef.current = null;
  };

  const handlePlayPause = () => {
    if (isPlaying) {
      stop();
    } else {
      start();
    }
    setIsPlaying((prev) => !prev);
  };

  const selectMode = (next: Mode) => {
    setMode(next);
    goToStep(0);
  };

  const step = GUIDED_STEPS[stepIndex];
  const expectation =
    mode === 'figureEight'
      ? describeFigureEightPosition(pose.position)
      : step.expect;

  return (
    <Container>
      <ScrollView contentContainerStyle={styles.scrollContent}>
        <Text style={styles.title}>PannerNode</Text>
        <Text style={styles.hint}>
          Use headphones. Only left/right balance and loudness are rendered.
        </Text>
        <Spacer.Vertical size={16} />

        <View style={styles.row}>
          {(['figureEight', 'guided'] as const).map((option) => (
            <Pressable
              key={option}
              style={[styles.chip, mode === option && styles.chipActive]}
              onPress={() => selectMode(option)}
            >
              <Text style={styles.chipText}>
                {option === 'figureEight' ? 'Figure 8' : 'Guided test'}
              </Text>
            </Pressable>
          ))}
        </View>
        <Spacer.Vertical size={12} />

        <View style={styles.row}>
          <Button
            onPress={handlePlayPause}
            title={isLoading ? 'Loading music...' : isPlaying ? 'Stop' : 'Play'}
            disabled={isLoading}
          />
        </View>
        <Spacer.Vertical size={20} />

        {mode === 'guided' && (
          <>
            <Text style={styles.section}>
              Step {stepIndex + 1}/{GUIDED_STEPS.length}: {step.title}
            </Text>
            <View style={styles.progressTrack}>
              <View
                style={[
                  styles.progressFill,
                  { width: `${stepProgress * 100}%` },
                ]}
              />
            </View>
            <Spacer.Vertical size={8} />
          </>
        )}

        <Text style={styles.expectLabel}>You should hear</Text>
        <Text style={styles.expect}>{expectation}</Text>
        <Text style={styles.facing}>{describeFacing(pose)}</Text>
        <Spacer.Vertical size={12} />
        <BalanceMeter balance={expectedBalance(pose.position)} />
        <Spacer.Vertical size={16} />

        <TopDownView pose={pose} showPath={mode === 'figureEight'} />
        <Spacer.Vertical size={16} />

        {mode === 'figureEight' ? (
          <Slider
            label="Lap (s)"
            value={periodSeconds}
            onValueChange={setPeriodSeconds}
            min={4}
            max={16}
            step={0.5}
          />
        ) : (
          <View style={styles.row}>
            <Pressable
              style={styles.chip}
              onPress={() => goToStep(stepIndex - 1)}
            >
              <Text style={styles.chipText}>Previous</Text>
            </Pressable>
            <Pressable style={styles.chip} onPress={() => goToStep(stepIndex)}>
              <Text style={styles.chipText}>Repeat</Text>
            </Pressable>
            <Pressable
              style={styles.chip}
              onPress={() => goToStep(stepIndex + 1)}
            >
              <Text style={styles.chipText}>Next</Text>
            </Pressable>
          </View>
        )}
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
    marginBottom: 8,
    opacity: 0.85,
  },
  expectLabel: {
    color: colors.white,
    opacity: 0.6,
    fontSize: 12,
    textTransform: 'uppercase',
  },
  expect: {
    color: colors.yellow,
    fontSize: 18,
    fontWeight: '600',
    marginTop: 4,
    minHeight: 48,
  },
  facing: {
    color: colors.white,
    opacity: 0.8,
    fontSize: 14,
    marginTop: 4,
  },
  row: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    justifyContent: 'center',
    gap: 8,
  },
  chip: {
    borderWidth: 1,
    borderColor: colors.border,
    borderRadius: layout.radius,
    paddingHorizontal: 12,
    paddingVertical: 8,
  },
  chipActive: {
    backgroundColor: colors.main,
    borderColor: colors.main,
  },
  chipText: {
    color: colors.white,
  },
  progressTrack: {
    height: 4,
    borderRadius: 2,
    backgroundColor: colors.backgroundLight,
    overflow: 'hidden',
  },
  progressFill: {
    height: '100%',
    backgroundColor: colors.main,
  },
  meterLabels: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 4,
  },
  meterLabel: {
    color: colors.white,
    opacity: 0.6,
    fontSize: 12,
  },
  meterTrack: {
    height: 12,
    borderRadius: 6,
    backgroundColor: colors.backgroundLight,
    justifyContent: 'center',
  },
  meterCentre: {
    position: 'absolute',
    left: '50%',
    width: 1,
    height: '100%',
    backgroundColor: colors.border,
  },
  meterMarker: {
    position: 'absolute',
    width: 12,
    height: 12,
    marginLeft: -6,
    borderRadius: 6,
    backgroundColor: colors.yellow,
  },
  view: {
    alignSelf: 'center',
    width: VIEW_SIZE,
    height: VIEW_SIZE,
    borderRadius: layout.radius,
    backgroundColor: colors.backgroundLight,
  },
  pathDot: {
    position: 'absolute',
    width: 2,
    height: 2,
    borderRadius: 1,
    backgroundColor: colors.border,
  },
  head: {
    position: 'absolute',
    left: VIEW_SIZE / 2 - 14,
    top: VIEW_SIZE / 2 - 14,
    width: 28,
    height: 28,
    borderRadius: 14,
    backgroundColor: colors.gray,
    alignItems: 'center',
  },
  nose: {
    position: 'absolute',
    top: -6,
    width: 8,
    height: 8,
    borderRadius: 4,
    backgroundColor: colors.gray,
  },
  earLeft: {
    position: 'absolute',
    left: -14,
    top: 6,
    color: colors.white,
    fontSize: 12,
  },
  earRight: {
    position: 'absolute',
    right: -14,
    top: 6,
    color: colors.white,
    fontSize: 12,
  },
  speaker: {
    position: 'absolute',
    width: SPEAKER_SIZE,
    height: SPEAKER_SIZE,
    alignItems: 'center',
    justifyContent: 'center',
  },
  speakerPointer: {
    position: 'absolute',
    top: 0,
    width: 0,
    height: 0,
    borderLeftWidth: 6,
    borderRightWidth: 6,
    borderBottomWidth: 9,
    borderLeftColor: 'transparent',
    borderRightColor: 'transparent',
    borderBottomColor: colors.yellow,
  },
  dot: {
    width: 14,
    height: 14,
    borderRadius: 7,
    backgroundColor: colors.yellow,
  },
  viewCaption: {
    position: 'absolute',
    bottom: 4,
    alignSelf: 'center',
    color: colors.white,
    opacity: 0.5,
    fontSize: 10,
  },
});

export default PannerNodeExample;
