import { useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import {
  WorkletNode,
  WorkletProcessingNode,
  WorkletSourceNode,
} from 'react-native-audio-worklets';
import Animated, {
  Extrapolation,
  interpolate,
  useAnimatedStyle,
  useSharedValue,
  withSpring,
  type SharedValue,
} from 'react-native-reanimated';
import { Button, Card, Container, Switch } from '../../components';
import { colors, typography } from '../../styles';
import { useAudioContext } from '../../utils/audio';

const HEAVY_JS_ITERATIONS = 4_000_000;

function runHeavyJsWork(): number {
  let acc = 0;
  for (let i = 0; i < HEAVY_JS_ITERATIONS; i++) {
    acc += Math.sin(i * 0.001) * Math.cos(i * 0.0013);
  }
  return acc;
}

function VisualizerBar({
  amplitude,
  index,
}: {
  amplitude: SharedValue<number>;
  index: number;
}) {
  const animatedStyle = useAnimatedStyle(() => {
    const centerIndex = 2;
    const distanceFromCenter = Math.abs(index - centerIndex);

    const height = interpolate(
      amplitude.value,
      [0, 1],
      [10, 200],
      Extrapolation.CLAMP
    );
    const red = interpolate(
      amplitude.value,
      [0, 1],
      [0, 255],
      Extrapolation.CLAMP
    );
    const green = interpolate(
      amplitude.value,
      [0, 1],
      [255, 0],
      Extrapolation.CLAMP
    );
    const opacity = 1 - distanceFromCenter * 0.15;

    return {
      height,
      backgroundColor: `rgba(${Math.floor(red)}, ${Math.floor(green)}, 0, ${opacity})`,
    };
  });

  return <Animated.View style={[styles.bar, animatedStyle]} />;
}

function Worklets() {
  const audioContext = useAudioContext();
  const workletSourceRef = useRef<WorkletSourceNode | null>(null);
  const workletProcessingRef = useRef<WorkletProcessingNode | null>(null);
  const workletNodeRef = useRef<WorkletNode | null>(null);
  const heavyWorkAccRef = useRef(0);
  const jsWorkloadTimerRef = useRef<ReturnType<typeof setInterval> | null>(
    null
  );

  const [isPlaying, setIsPlaying] = useState(false);
  const [heavyJsLoad, setHeavyJsLoad] = useState(false);

  const bar0 = useSharedValue(0);
  const bar1 = useSharedValue(0);
  const bar2 = useSharedValue(0);
  const bar3 = useSharedValue(0);
  const bar4 = useSharedValue(0);

  useEffect(
    () => () => {
      workletSourceRef.current?.disconnect();
      workletProcessingRef.current?.disconnect();
      workletNodeRef.current?.disconnect();
    },
    []
  );

  useEffect(() => {
    if (!heavyJsLoad) {
      if (jsWorkloadTimerRef.current != null) {
        clearInterval(jsWorkloadTimerRef.current);
        jsWorkloadTimerRef.current = null;
      }
      return;
    }

    jsWorkloadTimerRef.current = setInterval(() => {
      heavyWorkAccRef.current += runHeavyJsWork();
    }, 0);

    return () => {
      if (jsWorkloadTimerRef.current != null) {
        clearInterval(jsWorkloadTimerRef.current);
        jsWorkloadTimerRef.current = null;
      }
    };
  }, [heavyJsLoad]);

  const start = () => {
    if (isPlaying) {
      return;
    }

    const sampleRate = audioContext.sampleRate;

    workletSourceRef.current = new WorkletSourceNode(
      audioContext,
      (
        audioData,
        outputChannelCount,
        framesToProcess,
        currentTime,
        startOffset
      ) => {
        'worklet';

        const frequency = 440;

        for (let channel = 0; channel < outputChannelCount; channel++) {
          for (let i = 0; i < framesToProcess; i++) {
            const sampleTime = currentTime + (startOffset + i) / sampleRate;
            const phase = 2 * Math.PI * frequency * sampleTime;
            audioData[channel]![i] = Math.sin(phase) * 0.25;
          }
        }
      }
    );

    workletProcessingRef.current = new WorkletProcessingNode(
      audioContext,
      (
        inputData,
        outputData,
        inputChannelCount,
        outputChannelCount,
        framesToProcess,
        currentTime
      ) => {
        'worklet';

        const tremolo = 0.75 + 0.25 * Math.sin(2 * Math.PI * 2 * currentTime);

        for (let ch = 0; ch < outputChannelCount; ch++) {
          const input = inputData[Math.min(ch, inputChannelCount - 1)]!;
          const output = outputData[ch]!;

          for (let i = 0; i < framesToProcess; i++) {
            output[i] = Math.tanh(input[i]! * 1.2) * tremolo;
          }
        }
      }
    );

    workletNodeRef.current = new WorkletNode(
      audioContext,
      (audioData: Float32Array) => {
        'worklet';

        let sum = 0;

        for (let i = 0; i < audioData.length; i++) {
          sum += audioData[i]! * audioData[i]!;
        }
        const rms = Math.sqrt(sum / audioData.length);
        const scaledAmplitude = Math.min(rms * 4, 1);

        bar0.value = withSpring(bar1.value, { damping: 18, stiffness: 120 });
        bar1.value = withSpring(bar2.value, { damping: 18, stiffness: 120 });
        bar3.value = withSpring(bar2.value, { damping: 18, stiffness: 120 });
        bar4.value = withSpring(bar3.value, { damping: 18, stiffness: 120 });
        bar2.value = withSpring(scaledAmplitude, {
          damping: 18,
          stiffness: 120,
        });
      },
      { domain: 'time-domain', bufferLength: 1024 }
    );

    workletSourceRef.current.connect(workletProcessingRef.current);
    workletProcessingRef.current.connect(workletNodeRef.current);
    workletNodeRef.current.connect(audioContext.destination);

    workletSourceRef.current.start();

    if (audioContext.state === 'suspended') {
      audioContext.resume();
    }

    setIsPlaying(true);
  };

  const stop = () => {
    workletSourceRef.current?.stop();
    workletSourceRef.current?.disconnect();
    workletProcessingRef.current?.disconnect();
    workletNodeRef.current?.disconnect();

    workletSourceRef.current = null;
    workletProcessingRef.current = null;
    workletNodeRef.current = null;

    bar0.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar1.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar2.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar3.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar4.value = withSpring(0, { damping: 20, stiffness: 100 });

    setIsPlaying(false);
  };

  const barAmplitudes = [bar0, bar1, bar2, bar3, bar4];

  return (
    <Container style={styles.content}>
      <Text style={styles.subtitle}>
        WorkletSource → WorkletProcessing → WorkletNode → destination
      </Text>

      <View style={styles.toggleRow}>
        <Text style={styles.toggleLabel}>Heavy JS workload</Text>
        <Switch value={heavyJsLoad} onValueChange={setHeavyJsLoad} />
      </View>

      <Card style={styles.visualizer}>
        <View style={styles.barsContainer}>
          {barAmplitudes.map((amplitude, index) => (
            <VisualizerBar key={index} amplitude={amplitude} index={index} />
          ))}
        </View>
      </Card>

      <View style={styles.buttonsContainer}>
        <Button onPress={start} title="Start Playing" disabled={isPlaying} />
        <Button onPress={stop} title="Stop Playing" disabled={!isPlaying} />
      </View>
    </Container>
  );
}

const styles = StyleSheet.create({
  content: {
    gap: 24,
  },
  subtitle: {
    ...typography.hint,
    textAlign: 'center',
  },
  toggleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    paddingHorizontal: 8,
  },
  toggleLabel: {
    color: colors.textPrimary,
    fontSize: 14,
  },
  visualizer: {
    height: 250,
    justifyContent: 'flex-end',
    alignItems: 'center',
  },
  barsContainer: {
    flexDirection: 'row',
    alignItems: 'flex-end',
    justifyContent: 'center',
    gap: 8,
  },
  bar: {
    borderRadius: 20,
    minHeight: 10,
    width: 40,
  },
  buttonsContainer: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 20,
  },
});

export default Worklets;
