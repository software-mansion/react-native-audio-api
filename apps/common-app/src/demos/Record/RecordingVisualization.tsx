import {
  Canvas,
  Group,
  Path,
  Skia,
  useCanvasRef,
  useCanvasSize,
} from '@shopify/react-native-skia';
import React, { useEffect, useMemo, useRef } from 'react';
import { Dimensions, StyleSheet, View } from 'react-native';
import {
  WorkletAudioContext,
  WorkletNode,
} from 'react-native-audio-worklets';
import {
  cancelAnimation,
  Easing,
  SharedValue,
  useDerivedValue,
  useSharedValue,
  withRepeat,
  withTiming,
} from 'react-native-reanimated';

import { Spacer } from '../../components';
import { audioRecorder as Recorder } from '../../singletons';
import constants from './constants';
import TimeStream from './TimeStream';
import { RecordingState } from './types';

const { width: windowWidth } = Dimensions.get('window');

const defaultNumBars = Math.floor(windowWidth / constants.barStep);

const historyNumBars = Math.floor(
  windowWidth / (constants.historyBarWidth + constants.historyBarGap)
);

function getInitialWaveform() {
  return new Array(defaultNumBars * 2).fill(-1);
}

function getInitialHistory() {
  return new Array(historyNumBars * 10).fill(-1);
}

interface RecordingVisualizationProps {
  state: RecordingState;
}

interface DrawDefaultWaveformParams {
  normalized: number;
  canvasHeight: number;
  barHeights: number[];
  translateX: SharedValue<number>;
  lastIndex: SharedValue<number>;
  numBars: number;
}

interface DrawHistoryWaveformParams {
  normalized: number;
  lifetimeCanvasHeight: number;
  history: number[];
  historyHead: SharedValue<number>;
  durationMS: SharedValue<number>;
  historyMidpointMS: SharedValue<number>;
}

function drawDefaultWaveform(params: DrawDefaultWaveformParams) {
  'worklet';
  const { normalized, canvasHeight, barHeights, translateX, lastIndex, numBars } =
    params;

  if (canvasHeight <= 0 || numBars <= 0) {
    return barHeights;
  }

  const value = normalized * canvasHeight * 0.8;
  let currentIndex =
    barHeights.length / 2 -
    1 +
    Math.floor(-translateX.value / constants.barStep);

  barHeights[currentIndex] = value;

  if (lastIndex.value !== -1) {
    for (let i = lastIndex.value + 1; i < currentIndex; i++) {
      const vInter =
        barHeights[lastIndex.value] +
        ((value - barHeights[lastIndex.value]) * (i - lastIndex.value)) /
          (currentIndex - lastIndex.value);
      barHeights[i] = vInter;
      if (i > numBars / 2) {
        barHeights[i - numBars / 2] = vInter;
      }
    }
  }

  if (currentIndex > numBars / 2) {
    barHeights[currentIndex - numBars / 2] = value;
  }

  for (let i = 1; i <= 4; i++) {
    barHeights[(currentIndex + i) % barHeights.length] = -1;
  }

  lastIndex.value = currentIndex;
  return barHeights;
}

function drawHistoryWaveform(params: DrawHistoryWaveformParams) {
  'worklet';

  const {
    history,
    normalized,
    lifetimeCanvasHeight,
    historyHead,
    durationMS,
    historyMidpointMS,
  } = params;

  if (lifetimeCanvasHeight <= 0) {
    return history;
  }

  const value = normalized * lifetimeCanvasHeight * 0.8;
  history[historyHead.value] = value;
  historyHead.value += 1;

  // downsample if needed
  if (historyHead.value >= history.length) {
    const halfLength = history.length / 2;

    for (let i = 0; i < halfLength; i++) {
      history[i] = Math.max(history[2 * i], history[2 * i + 1]);
    }

    historyHead.value = halfLength;
    historyMidpointMS.value = durationMS.value;
  }

  return history;
}

const RecordingVisualization: React.FC<RecordingVisualizationProps> = ({
  state,
}) => {
  const canvasRef = useCanvasRef();
  const lifetimeCanvasRef = useCanvasRef();

  const { size } = useCanvasSize(canvasRef);
  const { size: lifetimeSize } = useCanvasSize(lifetimeCanvasRef);
  const barHeights = useSharedValue<number[]>(getInitialWaveform());

  const history = useSharedValue<number[]>(getInitialHistory());
  const historyHead = useSharedValue(0);
  const historyMidpointMS = useSharedValue(0);
  const historyRenderer = useSharedValue<number[]>(
    new Array(historyNumBars).fill(-1)
  );

  const translateX = useSharedValue(0);
  const lastIndex = useSharedValue(-1);
  const durationMS = useSharedValue(0);
  const canvasHeightSV = useSharedValue(0);
  const lifetimeCanvasHeightSV = useSharedValue(0);
  const numBarsSV = useSharedValue(0);

  const stateRef = useRef(state);
  const workletContextRef = useRef<WorkletAudioContext | null>(null);
  const workletNodeRef = useRef<WorkletNode | null>(null);
  const workletReadyRef = useRef(false);

  const numBars = useMemo(() => {
    if (size.width === 0) {
      return 0;
    }

    return Math.ceil(size.width / constants.barStep) * 2;
  }, [size.width]);

  const waveformPath = useDerivedValue(() => {
    const path = Skia.PathBuilder.Make().build();
    const currentHeights = barHeights.value;
    const canvasHeight = size.height;

    if (currentHeights.length === 0 || canvasHeight === 0) {
      return path;
    }

    currentHeights.forEach((height: number, index: number) => {
      if (height < 0) {
        return;
      }

      const x = index * constants.barStep + constants.barWidth / 2;
      const y1 = (canvasHeight - height) / 2;
      const y2 = (canvasHeight + height) / 2;

      path.moveTo(x, y1);
      path.lineTo(x, y2);
    });

    return path;
  }, [size, numBars]);

  const historyWaveformPath = useDerivedValue(() => {
    const path = Skia.PathBuilder.Make().build();
    const canvasHeight = lifetimeSize.height;
    const values = historyRenderer.value;

    if (historyHead.value < historyNumBars) {
      // render as it is
      for (let i = 0; i < historyHead.value; i++) {
        values[i] = history.value[i];

        if (values[i] < 0) {
          continue;
        }

        const x =
          i * (constants.historyBarWidth + constants.historyBarGap) +
          constants.historyBarWidth / 2;
        const y1 = (canvasHeight - values[i]) / 2;
        const y2 = (canvasHeight + values[i]) / 2;

        path.moveTo(x, y1);
        path.lineTo(x, y2);
      }

      return path;
    }

    const ratio = historyHead.value / historyNumBars;

    // render rest
    for (let i = 0; i < historyNumBars; i++) {
      let maxVal = -1;
      const startIndex = Math.floor(i * ratio);
      const endIndex = Math.floor((i + 1) * ratio);

      for (let j = startIndex; j < endIndex; j++) {
        if (history.value[j] > maxVal) {
          maxVal = history.value[j];
        }
      }

      values[i] = maxVal;

      if (values[i] < 0) {
        continue;
      }

      const x =
        i * (constants.historyBarWidth + constants.historyBarGap) +
        constants.historyBarWidth / 2;
      const y1 = (canvasHeight - values[i]) / 2;
      const y2 = (canvasHeight + values[i]) / 2;

      path.moveTo(x, y1);
      path.lineTo(x, y2);
    }

    return path;
  }, [lifetimeSize]);

  useEffect(() => {
    stateRef.current = state;
  }, [state]);

  useEffect(() => {
    numBarsSV.value = numBars;
    canvasHeightSV.value = size.height;
    lifetimeCanvasHeightSV.value = lifetimeSize.height;
  }, [numBars, size.height, lifetimeSize.height, numBarsSV, canvasHeightSV, lifetimeCanvasHeightSV]);

  useEffect(() => {
    if (numBars <= 0) {
      return;
    }

    if (barHeights.value.length !== numBars) {
      barHeights.value = new Array(numBars).fill(-1);
    }
  }, [numBars, barHeights]);

  useEffect(() => {
    if (workletContextRef.current != null) {
      return;
    }

    const workletContext = new WorkletAudioContext({
      sampleRate: constants.sampleRate,
    });
    const workletNode = new WorkletNode(
      workletContext,
      (audioData) => {
        'worklet';

        const canvasHeight = canvasHeightSV.value;
        const lifetimeCanvasHeight = lifetimeCanvasHeightSV.value;
        const activeNumBars = numBarsSV.value;

        if (canvasHeight <= 0 || activeNumBars <= 0) {
          return;
        }

        durationMS.value +=
          (audioData.length / constants.sampleRate) * 1000;

        let maxValue = 0;
        for (let i = 0; i < audioData.length; i++) {
          const val = Math.abs(audioData[i]!);
          if (val > maxValue) {
            maxValue = val;
          }
        }

        const db =
          maxValue > 0 ? 20 * Math.log10(maxValue) : constants.minDb;
        let normalized =
          (db - constants.minDb) / (constants.maxDb - constants.minDb);
        normalized = Math.max(0, Math.min(1, normalized));

        barHeights.modify(<T extends number[]>(heights: T) => {
          'worklet';

          return drawDefaultWaveform({
            normalized,
            canvasHeight,
            barHeights: heights,
            translateX,
            lastIndex,
            numBars: activeNumBars,
          }) as T;
        });

        history.modify(<T extends number[]>(hist: T) => {
          'worklet';

          return drawHistoryWaveform({
            normalized,
            lifetimeCanvasHeight,
            history: hist,
            historyHead,
            durationMS,
            historyMidpointMS,
          }) as T;
        });
      },
      {
        domain: 'time-domain',
        bufferLength: constants.workletBufferLength,
      }
    );

    workletContextRef.current = workletContext;
    workletNodeRef.current = workletNode;

    let cancelled = false;

    const startWorkletGraph = async () => {
      await workletContext.resume();
      if (cancelled) {
        return;
      }

      workletNode.connect(workletContext.destination);
      workletReadyRef.current = true;

      const currentState = stateRef.current;
      if (
        currentState === RecordingState.Recording ||
        currentState === RecordingState.Paused
      ) {
        Recorder.connect(workletContext, workletNode);
      }
    };

    startWorkletGraph();

    return () => {
      cancelled = true;
      workletReadyRef.current = false;
      Recorder.disconnect();
      workletNode.disconnect();
      workletContextRef.current = null;
      workletNodeRef.current = null;
      workletContext.close().catch(() => {});
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  useEffect(() => {
    const workletContext = workletContextRef.current;
    const workletNode = workletNodeRef.current;

    if (!workletReadyRef.current || !workletContext || !workletNode) {
      return;
    }

    const shouldRouteAudio =
      state === RecordingState.Recording || state === RecordingState.Paused;

    if (!shouldRouteAudio) {
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

  useEffect(() => {
    if (state === RecordingState.Recording) {
      const animationTarget = -size.width;
      const animationDuration = 1000 * (size.width / constants.pixelsPerSecond);

      translateX.value = withRepeat(
        withTiming(animationTarget, {
          duration: animationDuration,
          easing: Easing.linear,
        }),
        -1, // Infinite loop
        false // No reverse
      );
    } else if (state === RecordingState.Paused) {
      cancelAnimation(translateX);

      const currentIndexOffset = -translateX.value / constants.barStep;

      const newBarHeights = [...barHeights.value];

      for (let i = 0; i < newBarHeights.length; i++) {
        newBarHeights[i] =
          newBarHeights[
            (i + Math.floor(currentIndexOffset)) % newBarHeights.length
          ];
      }

      barHeights.value = newBarHeights;
      translateX.value = 0;
    } else {
      cancelAnimation(translateX);
      translateX.value = 0;
      barHeights.value = Array(numBars).fill(-1);
      historyRenderer.value = Array(historyNumBars).fill(-1);
      history.value = Array(historyNumBars * 10).fill(-1);
      historyHead.value = 0;
      historyMidpointMS.value = 0;
      durationMS.value = 0;
      lastIndex.value = -1;
    }
  }, [
    state,
    size,
    translateX,
    barHeights,
    numBars,
    durationMS,
    lastIndex,
    history,
    historyHead,
    historyMidpointMS,
    historyRenderer,
  ]);

  const transformPath = useDerivedValue(() => [
    {
      translateX: translateX.value,
    },
  ]);

  return (
    <>
      <View style={styles.container}>
        <Canvas style={styles.canvas} ref={canvasRef}>
          <Group transform={transformPath}>
            <Path
              path={waveformPath}
              style="stroke"
              strokeWidth={constants.barWidth}
              strokeCap="round"
              color="#ff6259"
            />
          </Group>
        </Canvas>
      </View>
      <View style={styles.timeStreamContainer}>
        <TimeStream
          isRecording={state === RecordingState.Recording}
          durationMS={durationMS}
        />
      </View>
      <Spacer.Vertical size={32} />
      <View style={styles.lifetimeContainer}>
        <Canvas style={styles.canvas} ref={lifetimeCanvasRef}>
          <Group>
            <Path
              path={historyWaveformPath}
              style="stroke"
              strokeWidth={constants.historyBarWidth}
              strokeCap="round"
              color="#ff6259"
            />
          </Group>
        </Canvas>
      </View>
    </>
  );
};
export default RecordingVisualization;

const styles = StyleSheet.create({
  container: {
    flex: 1,
    width: '100%',
    backgroundColor: 'rgba(0, 0, 0, 0.15)',
    flexDirection: 'column',
  },
  canvas: {
    flex: 1,
  },
  timeStreamContainer: {
    height: 20,
    marginTop: 8,
  },
  lifetimeContainer: {
    marginTop: 16,
    height: 75,
    width: '100%',
    backgroundColor: 'rgba(0, 0, 0, 0.15)',
    flexDirection: 'column',
  },
});
