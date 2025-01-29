import React, {
  useState,
  useEffect,
  useRef,
  useMemo,
} from 'react';
import * as FileSystem from 'expo-file-system';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
  AnalyserNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button, LayoutChangeEvent } from 'react-native';
import { Canvas as SKCanvas, vec, Line } from '@shopify/react-native-skia';

interface Size {
  width: number;
  height: number;
}

interface ChartProps {
  data: number[];
  dataSize: number;
}

const FrequencyChart: React.FC<ChartProps> = (props) => {
  const [size, setSize] = useState<Size>({ width: 0, height: 0 });
  const { data, dataSize } = props;

  const onCanvasLayout = (event: LayoutChangeEvent) => {
    const { width, height } = event.nativeEvent.layout;

    setSize({ width, height });
  };

  const barWidth = size.width / dataSize;

  const points = useMemo(() => {
    return data.map((value, index) => {
      const x = index * barWidth;
      const y1 = size.height;
      const y2 = size.height - size.height * (value / 256);

      const hue = (index / dataSize) * 360;
      const color = `hsl(${hue}, 100%, 50%)`;

      return { x1: x, y1, x2: x, y2, color };
    });
  }, [size, data, dataSize]);

  return (
    <SKCanvas style={{ flex: 1 }} onLayout={onCanvasLayout}>
      {points.map((point, index) => (
        <Line
          key={index}
          p1={vec(point.x1, point.y1)}
          p2={vec(point.x2, point.y2)}
          style="stroke"
          color={point.color}
          strokeWidth={barWidth}
        />
      ))}
    </SKCanvas>
  );
}

const FFT_SIZE = 512;

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [times, setTimes] = useState<number[]>(new Array(FFT_SIZE).fill(127));
  const [freqs, setFreqs] = useState<number[]>(new Array(FFT_SIZE / 2).fill(0));

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);

  const handlePlayPause = () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current || !analyserRef.current) {
        return
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBufferRef.current;
      bufferSourceRef.current.connect(analyserRef.current);

      bufferSourceRef.current.start();

      requestAnimationFrame(draw);
    }

    setIsPlaying((prev) => !prev);
  };

  const draw = () => {
    if (!analyserRef.current) {
      return;
    }

    const timesArrayLength = analyserRef.current.fftSize;
    const frequencyArrayLength = analyserRef.current.frequencyBinCount;

    const timesArray = new Array(timesArrayLength);
    analyserRef.current.getByteTimeDomainData(timesArray);
    setTimes(timesArray);

    const freqsArray = new Array(frequencyArrayLength);
    analyserRef.current.getByteFrequencyData(freqsArray);
    setFreqs(freqsArray);

    requestAnimationFrame(draw);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!analyserRef.current) {
      analyserRef.current = audioContextRef.current.createAnalyser();
      analyserRef.current.fftSize = FFT_SIZE;
      analyserRef.current.smoothingTimeConstant = 0.8;

      analyserRef.current.connect(audioContextRef.current.destination);
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      audioBufferRef.current = await FileSystem.downloadAsync(
        'https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-02.mp3',
        FileSystem.documentDirectory + 'audio.mp3'
      ).then(({ uri }) => {
        return audioContextRef.current!.decodeAudioDataSource(uri);
      });

      setIsLoading(false);
    };

    fetchBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <View style={{ flex: 1}}>
      <View style={{ flex: 0.2 }} />
      <FrequencyChart data={freqs} dataSize={FFT_SIZE / 2} />
      <View
        style={{ flex: 0.5, justifyContent: 'center', alignItems: 'center' }}>
        {isLoading && <ActivityIndicator color="#FFFFFF" />}
        <View
          style={{
            justifyContent: 'center',
            flexDirection: 'row',
            marginTop: 16,
          }}>
        <Button
            onPress={handlePlayPause}
            title={isPlaying ? 'Pause' : 'Play'}
            disabled={!audioBufferRef.current}
            color={'#38acdd'}
          />
        </View>
      </View>
    </View>
  );
};

export default AudioVisualizer;
