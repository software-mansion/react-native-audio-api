import React, { FC, useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import {
  AudioContext,
  NotSupportedError,
  type AudioParam,
  type OscillatorNode,
} from 'react-native-audio-api';

import { Button, Card } from '../../components';
import { colors, typography } from '../../styles';
import * as TestUI from '../../testComponents';
import type { ScenarioStatus, StepItem } from '../../testComponents';
import { monospace } from '../AudioIOPlayground/ui';
import { sleep } from '../sleep';

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

const POLL_INTERVAL_MS = 50;
const PAUSE_BETWEEN_CASES_MS = 300;

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

const idleStatuses = (): Record<string, PlayStatus> =>
  Object.fromEntries(AUDIO_TEST_CASES.map((tc) => [tc.key, 'idle']));

function runErrorSubCases(ctx: AudioContext): StepItem[] {
  const t0 = ctx.currentTime;
  return ERROR_SUB_CASES.map(({ id, label, run }): StepItem => {
    try {
      run(ctx, t0);
      return {
        id,
        message: label,
        status: 'fail',
        details: 'Expected NotSupportedError but nothing was thrown',
      };
    } catch (e) {
      if (e instanceof NotSupportedError) {
        return { id, message: label, status: 'pass' };
      }
      return {
        id,
        message: label,
        status: 'fail',
        details: `Unexpected error: ${e instanceof Error ? e.message : String(e)}`,
      };
    }
  });
}

const AudioParamPipeline: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const isUnmountedRef = useRef(false);
  const stopRequestedRef = useRef(false);
  const currentOscRef = useRef<OscillatorNode | null>(null);

  const [audioStatuses, setAudioStatuses] = useState(idleStatuses);
  const [errorSteps, setErrorSteps] = useState<StepItem[] | null>(null);
  const [isPlayingAll, setIsPlayingAll] = useState(false);

  useEffect(() => {
    isUnmountedRef.current = false;
    audioContextRef.current = new AudioContext();
    return () => {
      isUnmountedRef.current = true;
      audioContextRef.current?.close();
    };
  }, []);

  const isPlaying = Object.values(audioStatuses).includes('playing');
  const isRunning = isPlayingAll || isPlaying;

  const setStatus = (key: string, status: PlayStatus) => {
    if (!isUnmountedRef.current) {
      setAudioStatuses((prev) => ({ ...prev, [key]: status }));
    }
  };

  /** Plays one case until it ends or Stop is pressed; one case at a time. */
  const playTestCase = async (tc: AudioTestCase) => {
    const ctx = audioContextRef.current;
    if (!ctx || currentOscRef.current) return;

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

    const totalMs = tc.duration * 1000 + 150;
    for (let elapsed = 0; elapsed < totalMs; elapsed += POLL_INTERVAL_MS) {
      if (stopRequestedRef.current || isUnmountedRef.current) {
        try {
          osc.stop();
        } catch {}
        break;
      }
      await sleep(POLL_INTERVAL_MS);
    }

    gainNode.disconnect();
    currentOscRef.current = null;
    setStatus(tc.key, 'done');
  };

  const playOne = (tc: AudioTestCase) => {
    stopRequestedRef.current = false;
    playTestCase(tc);
  };

  const runErrorTest = () => {
    const ctx = audioContextRef.current;
    if (ctx) {
      setErrorSteps(runErrorSubCases(ctx));
    }
  };

  const playAll = async () => {
    stopRequestedRef.current = false;
    setIsPlayingAll(true);
    setAudioStatuses(idleStatuses());
    setErrorSteps(null);

    for (const tc of AUDIO_TEST_CASES) {
      await playTestCase(tc);
      if (stopRequestedRef.current) break;
      await sleep(PAUSE_BETWEEN_CASES_MS);
    }

    if (!stopRequestedRef.current && !isUnmountedRef.current) {
      runErrorTest();
    }
    if (!isUnmountedRef.current) {
      setIsPlayingAll(false);
    }
  };

  const stop = () => {
    stopRequestedRef.current = true;
    try {
      currentOscRef.current?.stop();
    } catch {}
  };

  const reset = () => {
    setAudioStatuses(idleStatuses());
    setErrorSteps(null);
  };

  const errorOverallStatus: ScenarioStatus | null = errorSteps
    ? errorSteps.every((s) => s.status === 'pass')
      ? 'pass'
      : 'fail'
    : null;
  const playedCount = Object.values(audioStatuses).filter(
    (s) => s === 'done'
  ).length;
  const passedCount =
    errorSteps?.filter((s) => s.status === 'pass').length ?? 0;
  const hasAnyResults = playedCount > 0 || errorSteps !== null;

  return (
    <TestUI.TestLayout
      description="Tap Play on each card to hear the automation. Pitch (oscillator frequency) is the animated parameter."
      summary={[
        {
          label: 'Status',
          value: isRunning ? 'running' : hasAnyResults ? 'done' : 'idle',
        },
        {
          label: 'Passed',
          value: errorSteps ? `${passedCount}/${ERROR_SUB_CASES.length}` : '—',
        },
        {
          label: 'Failed',
          value: errorSteps ? String(errorSteps.length - passedCount) : '—',
        },
      ]}
      actions={[
        {
          title: isPlayingAll ? 'Running…' : 'Play All',
          onPress: playAll,
          disabled: isRunning,
          minWidth: 110,
        },
        { title: 'Stop', onPress: stop, disabled: !isRunning, minWidth: 80 },
        { title: 'Reset', onPress: reset, disabled: isRunning, minWidth: 90 },
      ]}
    >
      <TestUI.SectionTitle title="Automation Methods" />

      {AUDIO_TEST_CASES.map((tc) => {
        const status = audioStatuses[tc.key];
        return (
          <Card key={tc.key}>
            <View style={styles.cardHeader}>
              <Text style={styles.cardTitle}>{tc.title}</Text>
              {status === 'playing' && (
                <Text style={[styles.statusLabel, styles.playingLabel]}>
                  ▶ PLAYING
                </Text>
              )}
              {status === 'done' && (
                <Text style={styles.statusLabel}>✓ PLAYED</Text>
              )}
            </View>
            <Text style={typography.hint}>{tc.description}</Text>
            {tc.events.map((line, i) => (
              <TestUI.StepRow
                key={i}
                step={{ id: String(i), message: line, status: 'info' }}
              />
            ))}
            <Button
              title={status === 'playing' ? 'Playing…' : 'Play'}
              onPress={() => playOne(tc)}
              disabled={isRunning}
              minWidth={80}
            />
          </Card>
        );
      })}

      <TestUI.SectionTitle title="Error Handling" />

      <Card>
        <View style={styles.cardHeader}>
          <Text style={styles.cardTitle}>NotSupportedError</Text>
          {errorOverallStatus && (
            <TestUI.StatusPill status={errorOverallStatus} />
          )}
        </View>
        <Text style={typography.hint}>
          Scheduling events that overlap a setValueCurveAtTime interval must
          throw NotSupportedError. Each sub-case uses a fresh GainNode.
        </Text>
        {errorSteps?.map((step) => (
          <TestUI.StepRow key={step.id} step={step} />
        ))}
        <Button title="Run Test" onPress={runErrorTest} disabled={isRunning} />
      </Card>
    </TestUI.TestLayout>
  );
};

export default AudioParamPipeline;

const styles = StyleSheet.create({
  cardHeader: {
    alignItems: 'center',
    flexDirection: 'row',
    gap: 8,
  },
  cardTitle: {
    color: colors.textPrimary,
    flex: 1,
    fontSize: 15,
    fontWeight: '700',
    fontFamily: monospace,
  },
  statusLabel: {
    color: colors.textMuted,
    fontSize: 11,
    fontWeight: '700',
  },
  playingLabel: {
    color: colors.blue,
  },
});
