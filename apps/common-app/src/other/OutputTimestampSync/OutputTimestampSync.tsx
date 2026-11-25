import React, { FC, useEffect, useRef, useState } from 'react';
import { Animated, Platform, StyleSheet, Text, View } from 'react-native';
import { AudioContext } from 'react-native-audio-api';
import type { AudioBuffer } from 'react-native-audio-api';

import { Card, Container, Select } from '../../components';
import { colors, typography } from '../../styles';
import * as TestUI from '../../testComponents';
import {
  CLICK_FREQUENCY_HZ,
  CSV_HEADER,
  LEAD_MS,
  SCHEDULING_METHODS,
  SCHEDULING_MODES,
  TRIAL_JITTER_MS,
  TRIAL_PERIOD_MS,
  createClickBuffer,
  meanAndSpread,
  methodForTrial,
  scheduleTimes,
  toCsvRow,
} from './trials';
import type { SchedulingMethod, SchedulingMode, Trial } from './trials';

/** Wake this early before the target, then poll frames until it passes. */
const FLASH_WAKE_EARLY_MS = 40;
const FLASH_FADE_MS = 120;
const LOG_LENGTH = 12;
/**
 * `playback` asks for the largest callback, which makes `currentTime`'s steps
 * easiest to see.
 */
const LATENCY_HINTS = ['interactive', 'balanced', 'playback'] as const;
type LatencyHint = (typeof LATENCY_HINTS)[number];

const OutputTimestampSync: FC = () => {
  const [mode, setMode] = useState<SchedulingMode>('alternate');
  const [latencyHint, setLatencyHint] = useState<LatencyHint>('interactive');
  const [isRunning, setIsRunning] = useState(false);
  const [trials, setTrials] = useState<Trial[]>([]);
  const [status, setStatus] = useState(
    'Film the screen in slow motion; each flash marks the moment its click should be heard.'
  );

  const flashOpacity = useRef(new Animated.Value(0)).current;
  const contextRef = useRef<AudioContext | null>(null);
  const clickBuffersRef = useRef<Record<SchedulingMethod, AudioBuffer> | null>(
    null
  );
  const timersRef = useRef<ReturnType<typeof setTimeout>[]>([]);
  const isRunningRef = useRef(false);
  const trialIndexRef = useRef(0);
  const modeRef = useRef(mode);
  modeRef.current = mode;

  useEffect(() => {
    // Moves the value to the native driver now, so the first flash is not set from JS.
    Animated.timing(flashOpacity, {
      toValue: 0,
      duration: 0,
      useNativeDriver: true,
    }).start();

    return stop;
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // The hint is fixed at construction, so each choice gets its own context.
  useEffect(() => {
    const context = new AudioContext({ latencyHint });
    contextRef.current = context;
    clickBuffersRef.current = Object.fromEntries(
      SCHEDULING_METHODS.map((method) => [
        method,
        createClickBuffer(context, CLICK_FREQUENCY_HZ[method]),
      ])
    ) as Record<SchedulingMethod, AudioBuffer>;

    return () => {
      contextRef.current = null;
      clickBuffersRef.current = null;
      context.close();
    };
  }, [latencyHint]);

  const after = (delayMs: number, callback: () => void) => {
    timersRef.current.push(setTimeout(callback, delayMs));
  };

  const flashAt = (targetPerfMs: number, trialIndex: number) => {
    const pollFrame = () => {
      if (!isRunningRef.current) {
        return;
      }
      if (performance.now() < targetPerfMs) {
        requestAnimationFrame(pollFrame);
        return;
      }

      const flashErrorMs = performance.now() - targetPerfMs;
      flashOpacity.setValue(1);
      Animated.timing(flashOpacity, {
        toValue: 0,
        duration: FLASH_FADE_MS,
        useNativeDriver: true,
      }).start();

      setTrials((previous) =>
        previous.map((trial) =>
          trial.index === trialIndex ? { ...trial, flashErrorMs } : trial
        )
      );
    };

    after(LEAD_MS - FLASH_WAKE_EARLY_MS, () =>
      requestAnimationFrame(pollFrame)
    );
  };

  const runTrial = () => {
    const context = contextRef.current;
    if (
      !isRunningRef.current ||
      context === null ||
      clickBuffersRef.current === null
    ) {
      return;
    }

    const targetPerfMs = performance.now() + LEAD_MS;
    const times = scheduleTimes(context, targetPerfMs);

    if (times === null) {
      setStatus('Waiting for the first rendered block…');
      after(100, runTrial);
      return;
    }

    const index = trialIndexRef.current++;
    const method = methodForTrial(modeRef.current, index);
    const when = times.when[method];

    const source = context.createBufferSource();
    source.buffer = clickBuffersRef.current[method];
    source.connect(context.destination);
    source.start(when);

    const trial: Trial = {
      index,
      method,
      targetPerfMs,
      when,
      offsetFromTimestampMs: {
        outputTimestamp: 0,
        latencyCompensated:
          (times.when.latencyCompensated - times.when.outputTimestamp) * 1000,
        currentTime:
          (times.when.currentTime - times.when.outputTimestamp) * 1000,
      },
      renderLeadMs: times.renderLeadMs,
      outputLatencyMs: times.outputLatencyMs,
    };
    setTrials((previous) => [...previous, trial]);
    setStatus(`Trial ${index}: ${method}`);
    flashAt(targetPerfMs, index);

    after(TRIAL_PERIOD_MS + Math.random() * TRIAL_JITTER_MS, runTrial);
  };

  const start = async () => {
    await contextRef.current?.resume();
    trialIndexRef.current = 0;
    setTrials([]);
    isRunningRef.current = true;
    setIsRunning(true);
    runTrial();
  };

  function stop() {
    isRunningRef.current = false;
    timersRef.current.forEach(clearTimeout);
    timersRef.current = [];
    setIsRunning(false);
  }

  const printCsv = () => {
    console.log([CSV_HEADER, ...trials.map(toCsvRow)].join('\n'));
    setStatus(`Printed ${trials.length} trials as CSV to the Metro console.`);
  };

  if (Platform.OS === 'web') {
    return (
      <Container centered>
        <TestUI.UnsupportedNotice
          title="Output Timestamp Sync"
          message="Needs the native engine's getOutputTimestamp."
        />
      </Container>
    );
  }

  const lastTrial = trials[trials.length - 1] as Trial | undefined;
  const compensatedOffset = meanAndSpread(
    trials.map((t) => t.offsetFromTimestampMs.latencyCompensated)
  );
  const currentTimeOffset = meanAndSpread(
    trials.map((t) => t.offsetFromTimestampMs.currentTime)
  );
  const renderLead = meanAndSpread(trials.map((t) => t.renderLeadMs));
  const flashError = meanAndSpread(
    trials.flatMap((t) =>
      t.flashErrorMs === undefined ? [] : [t.flashErrorMs]
    )
  );

  const formatStat = ({ mean, spread }: { mean: number; spread: number }) =>
    `${mean.toFixed(1)} ± ${spread.toFixed(1)} ms`;

  const deviceStats = [
    {
      label: 'currentTime − contextTime',
      value: `${renderLead.mean.toFixed(1)} ms`,
    },
    {
      label: 'outputLatency',
      value: `${(lastTrial?.outputLatencyMs ?? 0).toFixed(1)} ms`,
    },
    { label: 'Flash trigger error', value: formatStat(flashError) },
  ];

  const actions: TestUI.ControlAction[] = [
    { title: 'Start', onPress: start, disabled: isRunning, minWidth: 90 },
    { title: 'Stop', onPress: stop, disabled: !isRunning, minWidth: 90 },
    {
      title: 'Print CSV',
      onPress: printCsv,
      disabled: isRunning || trials.length === 0,
      minWidth: 120,
    },
  ];

  return (
    <View style={styles.screen}>
      <TestUI.TestLayout
        description={`Each trial aims a click and a screen flash at performance.now() + ${LEAD_MS} ms. The 2 kHz click uses getOutputTimestamp, 1 kHz currentTime − outputLatency, 500 Hz plain currentTime.`}
        summary={[
          { label: 'Trials', value: `${trials.length}` },
          { label: '−latency Δ', value: formatStat(compensatedOffset) },
          { label: 'currentTime Δ', value: formatStat(currentTimeOffset) },
        ]}
        currentStep={status}
        actions={actions}
      >
        <View style={styles.selectRow}>
          <Select value={mode} options={SCHEDULING_MODES} onChange={setMode} />
          <Select
            value={latencyHint}
            options={[...LATENCY_HINTS]}
            onChange={(hint) => {
              if (!isRunning) {
                setLatencyHint(hint);
              }
            }}
          />
        </View>

        {/* Large enough to read on slow-motion footage. */}
        <View style={styles.cameraLabel}>
          <Text style={styles.trialNumber}>{lastTrial?.index ?? '–'}</Text>
          <Text style={styles.trialMethod}>
            {lastTrial
              ? `${lastTrial.method} · ${CLICK_FREQUENCY_HZ[lastTrial.method]} Hz`
              : 'idle'}
          </Text>
        </View>

        <TestUI.SectionTitle title="Device" />
        <Card>
          {deviceStats.map((stat) => (
            <View key={stat.label} style={styles.statRow}>
              <Text style={typography.caption}>{stat.label}</Text>
              <Text style={styles.statValue}>{stat.value}</Text>
            </View>
          ))}
        </Card>

        <TestUI.LiveLog
          entries={trials
            .slice(-LOG_LENGTH)
            .reverse()
            .map(
              (t) =>
                `#${t.index} ${t.method}  −latency ${t.offsetFromTimestampMs.latencyCompensated.toFixed(1)} ms  flash ${t.flashErrorMs?.toFixed(1) ?? '…'} ms`
            )}
          emptyMessage="Trials appear here once started."
        />
      </TestUI.TestLayout>

      <Animated.View
        pointerEvents="none"
        style={[styles.flash, { opacity: flashOpacity }]}
      />
    </View>
  );
};

const styles = StyleSheet.create({
  screen: {
    flex: 1,
  },
  selectRow: {
    flexDirection: 'row',
    gap: 8,
  },
  cameraLabel: {
    alignItems: 'center',
  },
  trialNumber: {
    color: colors.textPrimary,
    fontSize: 72,
    fontWeight: '700',
  },
  trialMethod: {
    color: colors.yellow,
    fontSize: 20,
  },
  statRow: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    alignItems: 'center',
  },
  statValue: {
    color: colors.textPrimary,
    fontSize: 15,
    fontWeight: '700',
  },
  flash: {
    ...StyleSheet.absoluteFill,
    backgroundColor: colors.textPrimary,
  },
});

export default OutputTimestampSync;
