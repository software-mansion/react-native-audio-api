import React, { FC, useCallback, useEffect, useRef, useState } from 'react';
import { ScrollView, StyleSheet, Text, View } from 'react-native';
import { AudioContext } from 'react-native-audio-api';
import type { AudioParam } from 'react-native-audio-api';

import { Button, Container, Spacer } from '../../components';
import { colors, layout } from '../../styles';
import {
  SectionTitle,
  StatusPill,
  StepRow,
  TestUI,
  type ScenarioStatus,
  type StepItem,
  type SummaryItem,
} from '../../testComponents';
import { NotSupportedError } from '../../../../../packages/react-native-audio-api/src/errors';

// ---------------------------------------------------------------------------
// Data types
// ---------------------------------------------------------------------------

interface AudioTestCase {
  key: string;
  title: string;
  description: string;
  duration: number;
  events: string[];
  setup: (freq: AudioParam, t0: number) => void;
}

interface ErrorSubCase {
  id: string;
  label: string;
  run: (ctx: AudioContext, t0: number) => void;
}

type PlayStatus = 'idle' | 'playing' | 'done';

// ---------------------------------------------------------------------------
// Frequency automation test cases
// Oscillator frequency param is automated — pitch changes are easily heard.
// ---------------------------------------------------------------------------

const AUDIO_TEST_CASES: AudioTestCase[] = [
  {
    key: 'setValueAtTime',
    title: 'setValueAtTime',
    description:
      'Frequency jumps instantly to discrete pitches at scheduled times.',
    duration: 3.5,
    events: [
      't = 0.0 s   freq → 220 Hz  (A3)',
      't = 0.7 s   freq → 440 Hz  (A4)',
      't = 1.4 s   freq → 330 Hz  (E4)',
      't = 2.1 s   freq → 165 Hz  (E3)',
      't = 2.8 s   freq → 220 Hz  (A3)',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(220, t0 + 0.0);
      freq.setValueAtTime(440, t0 + 0.7);
      freq.setValueAtTime(330, t0 + 1.4);
      freq.setValueAtTime(165, t0 + 2.1);
      freq.setValueAtTime(220, t0 + 2.8);
    },
  },
  {
    key: 'linearRamp',
    title: 'linearRampToValueAtTime',
    description: 'Pitch sweeps up then down with a constant rate of change.',
    duration: 4.0,
    events: [
      't = 0.0 s    freq = 110 Hz',
      't = 0 → 2 s  linear ramp  →  880 Hz',
      't = 2 → 3.5s linear ramp  →  110 Hz',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(110, t0 + 0.0);
      freq.linearRampToValueAtTime(880, t0 + 2.0);
      freq.linearRampToValueAtTime(110, t0 + 3.5);
    },
  },
  {
    key: 'exponentialRamp',
    title: 'exponentialRampToValueAtTime',
    description:
      'Pitch sweeps up then down at a constant number of semitones per second — sounds like an even glide to the ear.',
    duration: 4.0,
    events: [
      't = 0.0 s    freq = 110 Hz',
      't = 0 → 2 s  exponential ramp  →  880 Hz  (+3 octaves)',
      't = 2 → 3.5s exponential ramp  →  110 Hz  (−3 octaves)',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(110, t0 + 0.0);
      freq.exponentialRampToValueAtTime(880, t0 + 2.0);
      freq.exponentialRampToValueAtTime(110, t0 + 3.5);
    },
  },
  {
    key: 'setTargetAtTime',
    title: 'setTargetAtTime',
    description:
      'Frequency approaches a target exponentially — like a natural portamento.',
    duration: 7.0,
    events: [
      't = 0.0 s   freq = 880 Hz',
      't = 0.5 s   → target 110 Hz   τ = 0.8 s  (descend)',
      't = 4.0 s   → target 880 Hz   τ = 0.8 s  (ascend)',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(880, t0 + 0.0);
      freq.setTargetAtTime(110, t0 + 0.5, 0.8);
      freq.setTargetAtTime(880, t0 + 4.0, 0.8);
    },
  },
  {
    key: 'setValueCurveAtTime',
    title: 'setValueCurveAtTime',
    description:
      'Frequency follows an arbitrary curve — plays a simple melody pattern.',
    duration: 4.0,
    events: [
      't = 0.0 s     freq = 220 Hz',
      't = 0.5 → 3s  curve [220, 440, 330, 660, 220, 880, 440]  over 2.5 s',
      't ≥ 3.0 s     holds at last value 440 Hz',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(220, t0 + 0.0);
      freq.setValueCurveAtTime(
        new Float32Array([220, 440, 330, 660, 220, 880, 440]),
        t0 + 0.5,
        2.5
      );
    },
  },
  {
    key: 'cancelScheduledValues',
    title: 'cancelScheduledValues',
    description:
      'Removes all future automation events. Frequency reverts to the last explicitly set value.',
    duration: 4.5,
    events: [
      't = 0.0 s   freq = 220 Hz  (setValueAtTime)',
      't = 0 → 4 s linear ramp  →  880 Hz',
      't = 2.0 s   cancelScheduledValues',
      '            ramp removed — freq snaps back to 220 Hz',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(220, t0 + 0.0);
      freq.linearRampToValueAtTime(880, t0 + 4.0);
      freq.cancelScheduledValues(t0 + 2.0);
    },
  },
  {
    key: 'cancelAndHoldAtTime',
    title: 'cancelAndHoldAtTime',
    description:
      'Removes future events and freezes frequency at its interpolated value at the cancel time.',
    duration: 4.5,
    events: [
      't = 0.0 s   freq = 220 Hz  (setValueAtTime)',
      't = 0 → 4 s linear ramp  →  880 Hz',
      't = 2.0 s   cancelAndHoldAtTime',
      '            ramp removed — freq holds at ~550 Hz',
    ],
    setup: (freq, t0) => {
      freq.setValueAtTime(220, t0 + 0.0);
      freq.linearRampToValueAtTime(880, t0 + 4.0);
      freq.cancelAndHoldAtTime(t0 + 2.0);
    },
  },
];

// ---------------------------------------------------------------------------
// NotSupportedError sub-cases
// Each uses a fresh GainNode so events don't accumulate across sub-cases.
// ---------------------------------------------------------------------------

const ERROR_SUB_CASES: ErrorSubCase[] = [
  {
    id: 'A',
    label: 'A: setValueCurveAtTime with event strictly inside interval',
    run: (ctx, t0) => {
      const gain = ctx.createGain().gain;
      gain.setValueAtTime(0.5, t0 + 0.5);
      gain.setValueCurveAtTime(new Float32Array([0, 1]), t0, 1);
    },
  },
  {
    id: 'B',
    label: 'B: setValueCurveAtTime overlapping exponentialRamp endTime',
    run: (ctx, t0) => {
      const gain = ctx.createGain().gain;
      gain.exponentialRampToValueAtTime(1, t0 + 0.5);
      gain.setValueCurveAtTime(new Float32Array([0, 1]), t0, 1);
    },
  },
  {
    id: 'C',
    label: 'C: two overlapping setValueCurveAtTime intervals',
    run: (ctx, t0) => {
      const gain = ctx.createGain().gain;
      gain.setValueCurveAtTime(new Float32Array([0, 1]), t0, 0.5);
      gain.setValueCurveAtTime(new Float32Array([0, 1]), t0 + 0.25, 0.5);
    },
  },
  {
    id: 'D',
    label: 'D: setValueAtTime inside an existing curve interval',
    run: (ctx, t0) => {
      const gain = ctx.createGain().gain;
      gain.setValueCurveAtTime(new Float32Array([0, 1]), t0, 1);
      gain.setValueAtTime(0.5, t0);
    },
  },
  {
    id: 'E',
    label: 'E: exponentialRamp endTime inside an existing curve interval',
    run: (ctx, t0) => {
      const gain = ctx.createGain().gain;
      gain.setValueCurveAtTime(new Float32Array([0, 1]), t0, 1);
      gain.exponentialRampToValueAtTime(1, t0 + 0.5);
    },
  },
];

// ---------------------------------------------------------------------------
// Component
// ---------------------------------------------------------------------------

const AudioParamPipeline: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const isUnmountedRef = useRef(false);
  const stopRequestedRef = useRef(false);
  const currentOscRef = useRef<ReturnType<AudioContext['createOscillator']> | null>(null);

  const [audioStatuses, setAudioStatuses] = useState<
    Record<string, PlayStatus>
  >(() => Object.fromEntries(AUDIO_TEST_CASES.map((tc) => [tc.key, 'idle'])));
  const [errorSteps, setErrorSteps] = useState<StepItem[] | null>(null);
  const [errorRunning, setErrorRunning] = useState(false);
  const [isPlayingAll, setIsPlayingAll] = useState(false);

  useEffect(() => {
    audioContextRef.current = new AudioContext();
    return () => {
      isUnmountedRef.current = true;
      audioContextRef.current?.close();
    };
  }, []);

  const setStatus = (key: string, status: PlayStatus) => {
    if (!isUnmountedRef.current) {
      setAudioStatuses((prev) => ({ ...prev, [key]: status }));
    }
  };

  const playTestCase = useCallback(async (tc: AudioTestCase) => {
    const ctx = audioContextRef.current;
    if (!ctx) return;

    setStatus(tc.key, 'playing');

    const osc = ctx.createOscillator();
    const gainNode = ctx.createGain();
    osc.connect(gainNode);
    gainNode.connect(ctx.destination);
    gainNode.gain.value = 0.4;

    const t0 = ctx.currentTime;
    tc.setup(osc.frequency, t0);
    osc.start(t0);
    osc.stop(t0 + tc.duration);
    currentOscRef.current = osc;

    const stepMs = 50;
    const totalMs = tc.duration * 1000 + 150;
    for (let elapsed = 0; elapsed < totalMs; elapsed += stepMs) {
      if (stopRequestedRef.current || isUnmountedRef.current) {
        try {
          osc.stop();
        } catch {}
        break;
      }
      await new Promise<void>((resolve) => setTimeout(resolve, stepMs));
    }

    currentOscRef.current = null;
    setStatus(tc.key, 'done');
  }, []);

  const runErrorTest = useCallback(async () => {
    const ctx = audioContextRef.current;
    if (!ctx || errorRunning) return;

    setErrorRunning(true);
    setErrorSteps(null);

    const t0 = ctx.currentTime;
    const steps: StepItem[] = ERROR_SUB_CASES.map((sc) => {
      try {
        sc.run(ctx, t0);
        return {
          id: sc.id,
          message: sc.label,
          status: 'fail' as const,
          details: 'Expected NotSupportedError but nothing was thrown',
        };
      } catch (e) {
        if (e instanceof NotSupportedError) {
          return { id: sc.id, message: sc.label, status: 'pass' as const };
        }
        return {
          id: sc.id,
          message: sc.label,
          status: 'fail' as const,
          details: `Unexpected error: ${e instanceof Error ? e.message : String(e)}`,
        };
      }
    });

    if (!isUnmountedRef.current) {
      setErrorSteps(steps);
      setErrorRunning(false);
    }
  }, [errorRunning]);

  const playAll = useCallback(async () => {
    if (isPlayingAll) return;
    stopRequestedRef.current = false;
    setIsPlayingAll(true);
    setAudioStatuses(
      Object.fromEntries(AUDIO_TEST_CASES.map((tc) => [tc.key, 'idle']))
    );
    setErrorSteps(null);

    for (const tc of AUDIO_TEST_CASES) {
      if (stopRequestedRef.current) break;
      await playTestCase(tc);
      if (stopRequestedRef.current) break;
      await new Promise<void>((resolve) => setTimeout(resolve, 300));
    }

    if (!stopRequestedRef.current) {
      await runErrorTest();
    }

    if (!isUnmountedRef.current) {
      setIsPlayingAll(false);
    }
    stopRequestedRef.current = false;
  }, [isPlayingAll, playTestCase, runErrorTest]);

  const stopAll = useCallback(() => {
    stopRequestedRef.current = true;
    try {
      currentOscRef.current?.stop();
    } catch {}
  }, []);

  const reset = useCallback(() => {
    if (isPlayingAll) return;
    setAudioStatuses(
      Object.fromEntries(AUDIO_TEST_CASES.map((tc) => [tc.key, 'idle']))
    );
    setErrorSteps(null);
  }, [isPlayingAll]);

  const controlActions = [
    {
      title: isPlayingAll ? 'Running…' : 'Play All',
      onPress: playAll,
      disabled: isPlayingAll,
      width: 110,
    },
    {
      title: 'Stop',
      onPress: stopAll,
      disabled: !isPlayingAll,
      width: 80,
    },
    {
      title: 'Reset',
      onPress: reset,
      disabled: isPlayingAll,
      width: 90,
    },
  ];

  const errorOverallStatus: ScenarioStatus | null = errorSteps
    ? errorSteps.every((s) => s.status === 'pass')
      ? 'pass'
      : 'fail'
    : null;

  const isRunning =
    isPlayingAll ||
    Object.values(audioStatuses).some((s) => s === 'playing') ||
    errorRunning;
  const playedCount = Object.values(audioStatuses).filter(
    (s) => s === 'done'
  ).length;
  const errorPassCount = errorSteps?.filter((s) => s.status === 'pass').length ?? 0;
  const errorFailCount = errorSteps?.filter((s) => s.status === 'fail').length ?? 0;

  const passedCount = playedCount + errorPassCount;
  const failedCount = errorFailCount;
  const totalCount = AUDIO_TEST_CASES.length + ERROR_SUB_CASES.length;
  const hasAnyResults = playedCount > 0 || errorSteps !== null;

  const summaryItems: SummaryItem[] = [
    {
      label: 'Status',
      value: isRunning ? 'running' : hasAnyResults ? 'done' : 'idle',
    },
    {
      label: 'Passed',
      value: hasAnyResults ? `${passedCount}/${totalCount}` : '—',
    },
    {
      label: 'Failed',
      value: hasAnyResults ? String(failedCount) : '—',
    },
  ];

  return (
    <Container disablePadding>
      <TestUI.Header
        title="AudioParam"
        subtitle="Tap Play on each card to hear the automation. Pitch (oscillator frequency) is the animated parameter."
      />
      <TestUI.Summary items={summaryItems} />
      <Spacer.Vertical size={12} />
      <TestUI.ControlBar actions={controlActions} />

      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.scrollContent}
      >
        <SectionTitle title="Automation Methods" />

        {AUDIO_TEST_CASES.map((tc) => {
          const status = audioStatuses[tc.key];
          return (
            <View key={tc.key} style={styles.card}>
              <View style={styles.cardHeader}>
                <Text style={styles.cardTitle}>{tc.title}</Text>
                {status === 'playing' && (
                  <Text style={styles.playingLabel}>▶ PLAYING</Text>
                )}
                {status === 'done' && <StatusPill status="pass" />}
              </View>
              <Text style={styles.description}>{tc.description}</Text>
              <View style={styles.eventList}>
                {tc.events.map((line, i) => (
                  <StepRow
                    key={i}
                    step={{ id: String(i), message: line, status: 'info' }}
                  />
                ))}
              </View>
              <Button
                title={status === 'playing' ? 'Playing…' : 'Play'}
                onPress={() => playTestCase(tc)}
                disabled={status === 'playing' || isPlayingAll}
                width={80}
              />
            </View>
          );
        })}

        <SectionTitle title="Error Handling" />

        <View style={styles.card}>
          <View style={styles.cardHeader}>
            <Text style={styles.cardTitle}>NotSupportedError</Text>
            {errorRunning && <Text style={styles.playingLabel}>▶ RUNNING</Text>}
            {errorOverallStatus && !errorRunning && (
              <StatusPill status={errorOverallStatus} />
            )}
          </View>
          <Text style={styles.description}>
            Scheduling events that overlap a setValueCurveAtTime interval must
            throw NotSupportedError. Each sub-case uses a fresh GainNode.
          </Text>
          {errorSteps && (
            <View style={styles.eventList}>
              {errorSteps.map((step) => (
                <StepRow key={step.id} step={step} />
              ))}
            </View>
          )}
          <Button
            title={errorRunning ? 'Running…' : 'Run Test'}
            onPress={runErrorTest}
            disabled={errorRunning || isPlayingAll}
            width={100}
          />
        </View>
      </ScrollView>
    </Container>
  );
};

export default AudioParamPipeline;

const styles = StyleSheet.create({
  scroll: {
    flex: 1,
  },
  scrollContent: {
    gap: 12,
    paddingHorizontal: 18,
    paddingTop: 12,
    paddingBottom: 32,
  },
  card: {
    backgroundColor: colors.backgroundLight,
    borderRadius: layout.radius,
    gap: 10,
    padding: 12,
  },
  cardHeader: {
    alignItems: 'center',
    flexDirection: 'row',
    justifyContent: 'space-between',
  },
  cardTitle: {
    color: colors.white,
    flex: 1,
    fontSize: 15,
    fontWeight: '700',
    fontFamily: 'Courier New',
    paddingRight: 8,
  },
  description: {
    color: colors.gray,
    fontSize: 12,
    lineHeight: 17,
  },
  eventList: {
    gap: 4,
  },
  playingLabel: {
    color: colors.main,
    fontSize: 11,
    fontWeight: '700',
  },
});
