import React, { FC, useEffect, useRef, useState } from 'react';
import {
  Animated,
  Platform,
  ScrollView,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import { AudioContext, type AudioBuffer } from 'react-native-audio-api';

import { Container, Select } from '../../components';
import { colors } from '../../styles';
import { TestUI, type ControlAction } from '../../testComponents';
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
  type SchedulingMethod,
  type SchedulingMode,
  type Trial,
} from './trials';

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

  const schedulingSummary = [
    { label: 'Trials', value: `${trials.length}` },
    {
      label: '− outputLatency vs timestamp',
      value: `${compensatedOffset.mean.toFixed(1)} ± ${compensatedOffset.spread.toFixed(1)} ms`,
    },
    {
      label: 'plain currentTime vs timestamp',
      value: `${currentTimeOffset.mean.toFixed(1)} ± ${currentTimeOffset.spread.toFixed(1)} ms`,
    },
  ];
  const deviceSummary = [
    {
      label: 'currentTime − contextTime',
      value: `${renderLead.mean.toFixed(1)} ms`,
    },
    {
      label: 'outputLatency',
      value: `${(lastTrial?.outputLatencyMs ?? 0).toFixed(1)} ms`,
    },
    {
      label: 'Flash trigger error',
      value: `${flashError.mean.toFixed(1)} ± ${flashError.spread.toFixed(1)} ms`,
    },
  ];

  const controls: ControlAction[] = [
    { title: 'Start', onPress: start, disabled: isRunning, width: 90 },
    { title: 'Stop', onPress: stop, disabled: !isRunning, width: 90 },
    {
      title: 'Print CSV',
      onPress: printCsv,
      disabled: isRunning || trials.length === 0,
      width: 120,
    },
  ];

  return (
    <Container disablePadding>
      <TestUI.Header
        title="Output Timestamp Sync"
        subtitle={`Each trial aims a click and a screen flash at performance.now() + ${LEAD_MS} ms. The 2 kHz click uses getOutputTimestamp, 1 kHz currentTime − outputLatency, 500 Hz plain currentTime.`}
      />
      <TestUI.CurrentStepCard message={status} />
      <TestUI.ControlBar actions={controls} />

      <View style={styles.modeRow}>
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

      <ScrollView contentContainerStyle={styles.scrollContent}>
        <TestUI.Summary items={schedulingSummary} />
        <TestUI.Summary items={deviceSummary} />
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
      </ScrollView>

      <Animated.View
        pointerEvents="none"
        style={[styles.flash, { opacity: flashOpacity }]}
      />
    </Container>
  );
};

const styles = StyleSheet.create({
  modeRow: {
    gap: 8,
    paddingHorizontal: 12,
    paddingBottom: 12,
  },
  cameraLabel: {
    alignItems: 'center',
    paddingVertical: 8,
  },
  trialNumber: {
    color: colors.white,
    fontSize: 72,
    fontWeight: '700',
  },
  trialMethod: {
    color: colors.yellow,
    fontSize: 20,
  },
  scrollContent: {
    gap: 12,
    paddingBottom: 32,
    paddingHorizontal: 18,
  },
  flash: {
    ...StyleSheet.absoluteFill,
    backgroundColor: colors.white,
  },
});

export default OutputTimestampSync;
