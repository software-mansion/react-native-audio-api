import React, {
  useState,
  useEffect,
  useRef,
  useMemo,
} from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
  AnalyserNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button, LayoutChangeEvent } from 'react-native';
import { Canvas as SKCanvas, vec, Points, SkPoint } from '@shopify/react-native-skia';

interface Size {
  width: number;
  height: number;
}

interface ChartProps {
  data: Uint8Array;
  dataSize: number;
}

const TimeChart: React.FC<ChartProps> = (props) => {
  const [size, setSize] = useState<Size>({ width: 0, height: 0 });
  const { data, dataSize } = props;

  const onCanvasLayout = (event: LayoutChangeEvent) => {
    const { width, height } = event.nativeEvent.layout;

    setSize({ width, height });
  };

  const points = useMemo(() => {
    const startWidth = 20;
    const maxWidth = size.width - 2 * startWidth;
    const maxHeight = size.height;

    const p: SkPoint[] = [];

    data.forEach((value, index) => {
      const x = startWidth + (index * maxWidth) / dataSize;
      const y = maxHeight - (value / 255) * maxHeight;

      p.push(vec(x, y));
    });

    return p;
  }, [size, data, dataSize]);

  return (
    <SKCanvas style={{ flex: 1 }} onLayout={onCanvasLayout}>
      <Points points={points} mode="polygon" color="#B5E1F1" strokeWidth={2} />
    </SKCanvas>
  );
}

const FFT_SIZE = 512;

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [times, setTimes] = useState<Uint8Array>(new Uint8Array(FFT_SIZE).fill(127));

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

    const timesArray = new Uint8Array(timesArrayLength);
    analyserRef.current.getByteTimeDomainData(timesArray);
    setTimes(timesArray);

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
      audioBufferRef.current = await fetch('https://software-mansion.github.io/react-native-audio-api/audio/music/example-music-02.mp3')
        .then((response) => response.arrayBuffer())
        .then((arrayBuffer) =>
          audioContextRef.current!.decodeAudioData(arrayBuffer)
        )

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
      <TimeChart data={times} dataSize={FFT_SIZE} />
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
