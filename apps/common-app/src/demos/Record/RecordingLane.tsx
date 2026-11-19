import {
  Canvas,
  matchFont,
  Picture,
  Skia,
  StrokeCap,
} from '@shopify/react-native-skia';
import { PaintStyle } from '@shopify/react-native-skia';
import React, { useEffect, useMemo, useRef, useState } from 'react';
import { LayoutChangeEvent, Platform, StyleSheet, View } from 'react-native';
import { WorkletAudioContext, WorkletNode } from 'react-native-audio-worklets';
import {
  FrameInfo,
  useDerivedValue,
  useFrameCallback,
  useSharedValue,
} from 'react-native-reanimated';

import { audioRecorder as Recorder } from '../../singletons';
import { colors, layout } from '../../styles';
import { formatDuration, levelFromAmplitude } from './levels';
import { RecordingState } from './types';

const sampleRate = 48000;
/** Samples per worklet callback (about 43 ms at 48 kHz). */
const chunkLength = 2048;
/** Callbacks folded into one bar, so a bar covers about 128 ms. */
const chunksPerBar = 3;
const barPeriodMs = ((chunkLength * chunksPerBar) / sampleRate) * 1000;

const barStep = 5.6;
const barWidth = 3;
/** Bars kept for drawing; more than the lane ever shows. */
const barCapacity = 256;

const waveHeight = 112;
const rulerHeight = 22;
const laneHeight = waveHeight + rulerHeight;
/** The playhead sits this far across the lane; the future is to its right. */
const playheadPosition = 0.71;
const rulerIntervalSeconds = 2;

const rulerFont = matchFont({
  fontFamily: Platform.select({ ios: 'Helvetica Neue', default: 'sans-serif' }),
  fontSize: 10,
});

interface RecordingLaneProps {
  state: RecordingState;
  isInterrupted: boolean;
}

/**
 * Live input level as capsule bars scrolling left past a fixed red playhead, on
 * a recessed lane with a time ruler. A worklet on the recorder's output folds
 * the input into one bar every `barPeriodMs`; the UI thread scrolls smoothly
 * between bars and stops when no new bar arrives (paused or interrupted).
 */
const RecordingLane: React.FC<RecordingLaneProps> = ({
  state,
  isInterrupted,
}) => {
  const [width, setWidth] = useState(0);

  const levels = useSharedValue<number[]>(new Array(barCapacity).fill(0));
  const barCount = useSharedValue(0);
  const pendingPeak = useSharedValue(0);
  const pendingChunks = useSharedValue(0);
  const isCapturing = useSharedValue(false);

  /** UI-thread clock and when the latest bar was first drawn. */
  const now = useSharedValue(0);
  const seenBarCount = useSharedValue(0);
  const seenBarAt = useSharedValue(0);

  const stateRef = useRef(state);
  const workletContextRef = useRef<WorkletAudioContext | null>(null);
  const workletNodeRef = useRef<WorkletNode | null>(null);
  const workletReadyRef = useRef(false);

  useEffect(() => {
    stateRef.current = state;
    isCapturing.value = state === RecordingState.Recording && !isInterrupted;

    if (state === RecordingState.Idle) {
      barCount.value = 0;
      seenBarCount.value = 0;
      pendingPeak.value = 0;
      pendingChunks.value = 0;
    }
  }, [
    state,
    isInterrupted,
    isCapturing,
    barCount,
    seenBarCount,
    pendingPeak,
    pendingChunks,
  ]);

  useEffect(() => {
    const workletContext = new WorkletAudioContext({ sampleRate });
    const workletNode = new WorkletNode(
      workletContext,
      (audioData) => {
        'worklet';

        if (!isCapturing.value) {
          return;
        }

        let peak = pendingPeak.value;
        for (let i = 0; i < audioData.length; i++) {
          peak = Math.max(peak, Math.abs(audioData[i]!));
        }
        pendingPeak.value = peak;
        pendingChunks.value += 1;

        if (pendingChunks.value < chunksPerBar) {
          return;
        }

        const slot = barCount.value % barCapacity;
        const level = levelFromAmplitude(peak);
        levels.modify(<T extends number[]>(values: T) => {
          'worklet';
          values[slot] = level;
          return values;
        });
        barCount.value += 1;
        pendingPeak.value = 0;
        pendingChunks.value = 0;
      },
      { domain: 'time-domain', bufferLength: chunkLength }
    );

    workletContextRef.current = workletContext;
    workletNodeRef.current = workletNode;
    let cancelled = false;

    (async () => {
      await workletContext.resume();
      if (cancelled) {
        return;
      }
      workletNode.connect(workletContext.destination);
      workletReadyRef.current = true;

      if (
        stateRef.current === RecordingState.Recording ||
        stateRef.current === RecordingState.Paused
      ) {
        Recorder.connect(workletContext, workletNode);
      }
    })();

    return () => {
      cancelled = true;
      workletReadyRef.current = false;
      Recorder.disconnect();
      workletNode.disconnect();
      workletContextRef.current = null;
      workletNodeRef.current = null;
      workletContext.close().catch(() => {});
    };
    // The worklet graph lives as long as the screen; the effect below routes
    // the recorder into it.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    const workletContext = workletContextRef.current;
    const workletNode = workletNodeRef.current;
    if (!workletReadyRef.current || !workletContext || !workletNode) {
      return;
    }

    if (state !== RecordingState.Recording && state !== RecordingState.Paused) {
      Recorder.disconnect();
      return;
    }

    workletContext.resume().then(() => {
      Recorder.connect(workletContext, workletNode);
    });

    return () => {
      Recorder.disconnect();
    };
  }, [state]);

  useFrameCallback((frame: FrameInfo) => {
    now.value = frame.timestamp;
    if (barCount.value !== seenBarCount.value) {
      seenBarCount.value = barCount.value;
      seenBarAt.value = frame.timestamp;
    }
  });

  const paints = useMemo(() => {
    const bar = Skia.Paint();
    bar.setAntiAlias(true);
    bar.setStyle(PaintStyle.Stroke);
    bar.setStrokeCap(StrokeCap.Round);
    bar.setStrokeWidth(barWidth);
    bar.setColor(Skia.Color(colors.textPrimary));
    bar.setAlphaf(0.85);

    const dot = Skia.Paint();
    dot.setAntiAlias(true);
    dot.setColor(Skia.Color(colors.textPrimary));
    dot.setAlphaf(0.25);

    const playhead = Skia.Paint();
    playhead.setAntiAlias(true);
    playhead.setColor(Skia.Color(colors.accent));
    playhead.setStrokeWidth(2);

    const label = Skia.Paint();
    label.setAntiAlias(true);
    label.setColor(Skia.Color(colors.textMuted));

    return { bar, dot, playhead, label };
  }, []);

  const picture = useDerivedValue(() => {
    const recorder = Skia.PictureRecorder();
    const canvas = recorder.beginRecording(
      Skia.XYWHRect(0, 0, width, laneHeight)
    );
    if (width === 0) {
      return recorder.finishRecordingAsPicture();
    }

    const playheadX = width * playheadPosition;
    const count = seenBarCount.value;
    const sinceLastBar = Math.min(
      1,
      (now.value - seenBarAt.value) / barPeriodMs
    );
    // Recording time at the playhead, in bars; it eases toward the next bar
    // while it is being captured, and holds when none arrives.
    const head = count === 0 && !isCapturing.value ? 0 : count + sinceLastBar;
    const xAt = (bars: number) => playheadX - (head - bars) * barStep;
    const centreY = waveHeight / 2;

    const firstVisible = Math.max(
      0,
      count - barCapacity,
      Math.floor(head - playheadX / barStep) - 1
    );
    for (let bar = firstVisible; bar < count; bar += 1) {
      const x = xAt(bar + 0.5);
      if (x > playheadX - barWidth) {
        continue;
      }
      const halfHeight =
        (levels.value[bar % barCapacity] * waveHeight * 0.8) / 2;
      canvas.drawLine(
        x,
        centreY - halfHeight,
        x,
        centreY + halfHeight,
        paints.bar
      );
    }

    for (let slot = Math.ceil(head) + 1; ; slot += 1) {
      const x = xAt(slot + 0.5);
      if (x > width) {
        break;
      }
      if (x > playheadX + barStep / 2) {
        canvas.drawCircle(x, centreY, 1.4, paints.dot);
      }
    }

    canvas.drawLine(playheadX, 4, playheadX, waveHeight - 4, paints.playhead);
    canvas.drawCircle(playheadX, 5, 4, paints.playhead);

    const barsPerSecond = 1000 / barPeriodMs;
    const visibleSeconds = width / barStep / barsPerSecond;
    const headSeconds = head / barsPerSecond;
    const firstMark =
      Math.max(
        0,
        Math.floor((headSeconds - visibleSeconds) / rulerIntervalSeconds)
      ) * rulerIntervalSeconds;
    for (
      let second = firstMark;
      second <= headSeconds + visibleSeconds;
      second += rulerIntervalSeconds
    ) {
      const label = formatDuration(second);
      const labelWidth = rulerFont.measureText(label).width;
      const x = xAt(second * barsPerSecond) - labelWidth / 2;
      if (x < 6 || x + labelWidth > width - 6) {
        continue;
      }
      canvas.drawText(label, x, waveHeight + 13, paints.label, rulerFont);
    }

    return recorder.finishRecordingAsPicture();
  });

  const onLayout = (event: LayoutChangeEvent) => {
    setWidth(event.nativeEvent.layout.width);
  };

  return (
    <View style={styles.lane} onLayout={onLayout}>
      <Canvas style={styles.canvas}>
        <Picture picture={picture} />
      </Canvas>
    </View>
  );
};

export default RecordingLane;

const styles = StyleSheet.create({
  lane: {
    height: laneHeight,
    borderRadius: layout.cardRadius,
    overflow: 'hidden',
    backgroundColor: colors.recess,
  },
  canvas: {
    flex: 1,
  },
});
