import { useEffect, useRef, useState } from 'react';
import { StyleSheet, Text, View } from 'react-native';
import {
  AudioContext,
  OscillatorNode,
} from 'react-native-audio-api';
import { WorkletNode } from 'react-native-audio-worklets';
import Animated, {
  Extrapolation,
  interpolate,
  useAnimatedStyle,
  useSharedValue,
  withSpring,
  type SharedValue,
} from 'react-native-reanimated';
import { Button, Container, Switch } from '../../components';
import { colors } from '../../styles';

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
  const audioContextRef = useRef<AudioContext | null>(null);
  const workletNodeRef = useRef<WorkletNode | null>(null);
  const oscillatorRef = useRef<OscillatorNode | null>(null);
  const lfoRef = useRef<OscillatorNode | null>(null);
  const heavyWorkAccRef = useRef(0);
  const jsWorkloadTimerRef = useRef<ReturnType<typeof setInterval> | null>(null);

  const [isPlaying, setIsPlaying] = useState(false);
  const [heavyJsLoad, setHeavyJsLoad] = useState(false);

  const bar0 = useSharedValue(0);
  const bar1 = useSharedValue(0);
  const bar2 = useSharedValue(0);
  const bar3 = useSharedValue(0);
  const bar4 = useSharedValue(0);

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

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
    const ctx = audioContextRef.current;
    if (isPlaying || !ctx) {
      return;
    }

    workletNodeRef.current = new WorkletNode(
      ctx,
      (audioBuffers, numberOfChannels) => {
        'worklet';
        if (numberOfChannels < 1) {
          return;
        }
        const buffer = audioBuffers[0];
        if (buffer == null) {
          return;
        }
        const channel = new Float32Array(buffer);
        let sum = 0;
        for (let i = 0; i < channel.length; i++) {
          sum += channel[i] * channel[i];
        }
        const rms = Math.sqrt(sum / channel.length);
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
      1024
    );

    const oscillator = ctx.createOscillator();
    oscillator.frequency.value = 440;

    const lfo = ctx.createOscillator();
    lfo.frequency.value = 2;
    const lfoGain = ctx.createGain();
    lfoGain.gain.value = 0.18;
    const amp = ctx.createGain();
    amp.gain.value = 0.25;

    lfo.connect(lfoGain);
    lfoGain.connect(amp.gain);
    oscillator.connect(amp);
    amp.connect(workletNodeRef.current);
    workletNodeRef.current.connect(ctx.destination);

    oscillator.start();
    lfo.start();

    oscillatorRef.current = oscillator;
    lfoRef.current = lfo;

    if (ctx.state === 'suspended') {
      ctx.resume();
    }

    setIsPlaying(true);
  };

  const stop = () => {
    oscillatorRef.current?.stop();
    lfoRef.current?.stop();
    workletNodeRef.current?.disconnect();

    oscillatorRef.current = null;
    lfoRef.current = null;
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
    <Container>
      <Text style={{ ...styles.title, color: colors.white }}>
        Audio Worklets Visualizer
      </Text>
      <Text style={{ ...styles.subtitle, color: colors.white }}>
        Oscillator → WorkletNode (RMS on UI runtime) → destination
      </Text>

      <View style={styles.toggleRow}>
        <Text style={styles.toggleLabel}>Heavy JS workload</Text>
        <Switch value={heavyJsLoad} onValueChange={setHeavyJsLoad} />
      </View>

      <View style={{ ...styles.visualizer, backgroundColor: colors.white }}>
        <View style={styles.barsContainer}>
          {barAmplitudes.map((amplitude, index) => (
            <VisualizerBar key={index} amplitude={amplitude} index={index} />
          ))}
        </View>
      </View>

      <View style={styles.buttonsContainer}>
        <Button onPress={start} title="Start Playing" disabled={isPlaying} />
        <Button onPress={stop} title="Stop Playing" disabled={!isPlaying} />
      </View>
    </Container>
  );
}

const styles = StyleSheet.create({
  title: {
    fontSize: 20,
    fontWeight: 'bold',
    marginBottom: 10,
    textAlign: 'center',
  },
  subtitle: {
    fontSize: 14,
    marginBottom: 24,
    textAlign: 'center',
  },
  toggleRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    marginBottom: 16,
    paddingHorizontal: 8,
  },
  toggleLabel: {
    color: colors.white,
    fontSize: 14,
  },
  visualizer: {
    height: 250,
    justifyContent: 'flex-end',
    alignItems: 'center',
    marginVertical: 16,
    borderRadius: 10,
    padding: 20,
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
