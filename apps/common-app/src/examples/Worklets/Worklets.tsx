import { useEffect, useRef } from "react";
import { Text, View, StyleSheet } from 'react-native';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
  RecorderAdapterNode,
  WorkletNode
} from 'react-native-audio-api';
import { Container, Button } from "../../components";
import { Extrapolation, useSharedValue } from "react-native-reanimated";
import Animated, {
  useAnimatedStyle,
  withSpring,
  interpolate,
} from "react-native-reanimated";
import { colors } from "../../styles";


function Worklets() {
  const SAMPLE_RATE = 44100;
  const recorderRef = useRef<AudioRecorder | null>(null);
  const aCtxRef = useRef<AudioContext | null>(null);
  const recorderAdapterRef = useRef<RecorderAdapterNode | null>(null);
  const workletNodeRef = useRef<WorkletNode | null>(null);

  const bar0 = useSharedValue(0);
  const bar1 = useSharedValue(0);
  const bar2 = useSharedValue(0); // center bar
  const bar3 = useSharedValue(0);
  const bar4 = useSharedValue(0);

  useEffect(() => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'spokenAudio',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });

    AudioManager.requestRecordingPermissions();
    recorderRef.current = new AudioRecorder({
      sampleRate: SAMPLE_RATE,
      bufferLengthInSamples: 1024,
    });
  }, []);

  const start = () => {
    if (!recorderRef.current) {
      console.error("Recorder is not initialized");
      return;
    }

    const worklet = (audioData: Array<Float32Array>, inputChannelCount: number) => {
      'worklet';


      // Calculates RMS amplitude
      let sum = 0;
      for (let i = 0; i < audioData.length; i++) {
        sum += audioData[0][i] * audioData[0][i];
      }
      const rms = Math.sqrt(sum / audioData[0].length);
      const scaledAmplitude = Math.min(rms * 1000, 1);

      console.log(`RMS: ${rms}, Scaled: ${scaledAmplitude}`);

      bar0.value = withSpring(bar1.value, { damping: 20, stiffness: 150 });
      bar1.value = withSpring(bar2.value, { damping: 20, stiffness: 150 });
      bar3.value = withSpring(bar2.value, { damping: 20, stiffness: 150 });
      bar4.value = withSpring(bar3.value, { damping: 20, stiffness: 150 });
      bar2.value = withSpring(scaledAmplitude, { damping: 20, stiffness: 200 });
    };

    aCtxRef.current = new AudioContext({ sampleRate: SAMPLE_RATE });
    recorderAdapterRef.current = aCtxRef.current.createRecorderAdapter();
    workletNodeRef.current = aCtxRef.current.createWorkletNode(worklet, 512, 1);
    recorderAdapterRef.current.connect(workletNodeRef.current);
    workletNodeRef.current.connect(aCtxRef.current.destination);

    recorderRef.current.connect(recorderAdapterRef.current);
    recorderRef.current.start();
    console.log("Recording started");

    if (aCtxRef.current.state === 'suspended') {
      aCtxRef.current.resume();
    }
  }

  const stop = () => {
    if (!recorderRef.current) {
      console.error("Recorder is not initialized");
      return;
    }
    recorderRef.current.stop();
    recorderAdapterRef.current = null;
    aCtxRef.current = null;
    console.log("Recording stopped");
    bar0.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar1.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar2.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar3.value = withSpring(0, { damping: 20, stiffness: 100 });
    bar4.value = withSpring(0, { damping: 20, stiffness: 100 });
  }

  const createBarStyle = (index: number) => {
    return useAnimatedStyle(() => {
      let amplitude = 0;

      switch (index) {
        case 0: amplitude = bar0.value; break;
        case 1: amplitude = bar1.value; break;
        case 2: amplitude = bar2.value; break;
        case 3: amplitude = bar3.value; break;
        case 4: amplitude = bar4.value; break;
      }

      const centerIndex = 2;
      const distanceFromCenter = Math.abs(index - centerIndex);

      const height = interpolate(
        amplitude,
        [0, 1],
        [10, 200],
        Extrapolation.CLAMP
      );

      // Interpolate red component: 0 (quiet) -> 255 (loud)
      const red = interpolate(
        amplitude,
        [0, 1],
        [0, 255],
        Extrapolation.CLAMP
      );

      // Interpolate green component: 255 (quiet) -> 0 (loud)
      const green = interpolate(
        amplitude,
        [0, 1],
        [255, 0],
        Extrapolation.CLAMP
      );

      const barWidth = 40 - (distanceFromCenter * 5);
      const opacity = 1 - (distanceFromCenter * 0.15);

      return {
        height,
        width: barWidth,
        backgroundColor: `rgba(${Math.floor(red)}, ${Math.floor(green)}, 0, ${opacity})`,
      };
    });
  };

  return (
    <Container>
      <Text style={{ ...styles.title, color: colors.white}}>Audio Worklets Visualizer</Text>
      <Text style={{ ...styles.subtitle, color: colors.white}}>Speak into the microphone to see the animation</Text>

      <View style={{...styles.visualizer, backgroundColor: colors.white}}>
        <View style={styles.barsContainer}>
          {Array.from({ length: 5 }, (_, index) => (
            <Animated.View
              key={index}
              style={[styles.bar, createBarStyle(index)]}
            />
          ))}
        </View>
      </View>

      <View style={styles.buttonsContainer}>
        <Button onPress={start} title="Start Recording" />
        <Button onPress={stop} title="Stop Recording" />
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
    marginBottom: 30,
    textAlign: 'center',
  },
  visualizer: {
    height: 250,
    justifyContent: 'flex-end',
    alignItems: 'center',
    marginVertical: 30,
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
  },
  buttonsContainer: {
    flexDirection: 'row',
    justifyContent: 'center',
    gap: 20,
  }
});

export default Worklets;
