import React, {
  useState,
  useEffect,
  useRef,
} from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
  AnalyserNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button } from 'react-native';

interface ChartProps {
  data: Uint8Array;
  dataSize: number;
}

const TimeChart: React.FC<ChartProps> = (props) => {
  const { data, dataSize } = props;

  const canvasRef = useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');

    const maxWidth = canvas.width;
    const maxHeight = canvas.height;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.beginPath();

    data.forEach((value, index) => {
      const x = (index * maxWidth) / dataSize;
      const y = maxHeight - (value / 255) * maxHeight;
      ctx.lineTo(x, y);
    });

    ctx.strokeStyle = '#B5E1F1';
    ctx.lineWidth = 2;
    ctx.stroke();
  }, [data, dataSize]);

  return (
    <canvas ref={canvasRef} style={{flex: 1}}></canvas>
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


  const handlePlayPause = async () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current || !analyserRef.current) {
        return
      }

      bufferSourceRef.current = await audioContextRef.current.createBufferSource();
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
      audioBufferRef.current = await fetch('/react-native-audio-api/audio/music/example-music-02.mp3')
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
    <View>
      <View style={{ flex: 0.2 }} />
      <TimeChart data={times} dataSize={FFT_SIZE} />
      <View
        style={{ flex: 0.5, justifyContent: 'center', alignItems: 'center' }}>
        {isLoading && <ActivityIndicator color="#FFFFFF" />}
        <View style={{
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

