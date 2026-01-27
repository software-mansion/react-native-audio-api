import React, {
  useState,
  useEffect,
  useRef,
  useCallback,
} from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
  AnalyserNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button } from 'react-native';
import CanvasSizer from './CanvasSizerComponent';

interface ChartProps {
  data: Uint8Array;
  dataSize: number;
}

const TimeChart: React.FC<ChartProps> = (props) => {
  const { data, dataSize } = props;

  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    if (!canvas) return;

    const dpr =
      typeof window !== 'undefined' && window.devicePixelRatio
        ? window.devicePixelRatio
        : 1;

    const cssWidth = Number(canvas.dataset.cssWidth) || canvas.clientWidth || 300;
    const cssHeight = Number(canvas.dataset.cssHeight) || canvas.clientHeight || 150;

    const scaledWidth = Math.max(1, Math.floor(cssWidth * dpr));
    const scaledHeight = Math.max(1, Math.floor(cssHeight * dpr));

    if (canvas.width !== scaledWidth) canvas.width = scaledWidth;
    if (canvas.height !== scaledHeight) canvas.height = scaledHeight;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.beginPath();

    const maxWidth = canvas.width;
    const maxHeight = canvas.height;

    for (let i = 0; i < data.length && i < dataSize; i++) {
      const value = data[i];
      const x = (i * maxWidth) / dataSize;
      const y = maxHeight - (value / 255) * maxHeight;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }

    // Keep stroke thickness consistent in CSS pixels by scaling with dpr
    ctx.strokeStyle = '#B5E1F1';
    ctx.lineWidth = 2 * dpr;
    ctx.stroke();
  }, [data, dataSize]);

  return (
    <CanvasSizer canvasHeight={300}>
      {({ width, height }) => {
        const dpr =
          typeof window !== 'undefined' && window.devicePixelRatio
            ? window.devicePixelRatio
            : 1;
        const scaledWidth = Math.max(1, Math.floor(width * dpr));
        const scaledHeight = Math.max(1, Math.floor(height * dpr));

        const containerStyle: React.CSSProperties = {
          width: `${width}px`,
          height: `${height}px`,
          overflow: 'hidden',
        };

        const canvasStyle: React.CSSProperties = {
          width: `${scaledWidth}px`,
          height: `${scaledHeight}px`,
          transform: `scale(${1 / dpr})`,
          transformOrigin: 'top left',
          display: 'block',
        };

        return (
          <div style={containerStyle}>
            <canvas
              ref={canvasRef}
              width={scaledWidth}
              height={scaledHeight}
              data-css-width={width}
              data-css-height={height}
              style={canvasStyle}
            />
          </div>
        );
      }}
    </CanvasSizer>
  );
}

const FFT_SIZE = 512;

const AudioVisualizer: React.FC = () => {
  const isPlayingRef = useRef(false);
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

  useEffect(() => {
    isPlayingRef.current = isPlaying;
  }, [isPlaying]);

  const draw = () => {
    if (!analyserRef.current) {
      return;
    }

    const timesArrayLength = analyserRef.current.fftSize;

    const timesArray = new Uint8Array(timesArrayLength);
    analyserRef.current.getByteTimeDomainData(timesArray);
    if (isPlayingRef.current) {
      setTimes(timesArray);
    }

    requestAnimationFrame(draw);
  }

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

