import React, { useState, useEffect, useRef } from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
  BiquadFilterNode,
} from 'react-native-audio-api';
import {
  View,
  Button,
  Text,
  Pressable,
  StyleSheet,
  ActivityIndicator,
  ScrollView,
} from 'react-native';

const FILTER_TYPES: BiquadFilterType[] = [
  'allpass', 'lowpass', 'highpass', 'bandpass',
  'lowshelf', 'highshelf', 'peaking', 'notch',
];

const RangeSlider = ({
  label,
  value,
  min,
  max,
  step,
  onChange,
  unit = '',
}: {
  label: string;
  value: number;
  min: number;
  max: number;
  step: number;
  unit?: string;
  onChange: (v: number) => void;
}) => (
  <div style={{ marginTop: 8 }}>
    <label style={{ color: '#fff' }}>
      {label}: {value.toFixed(1)} {unit}
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        style={{ width: '100%', marginTop: 4 }}
      />
    </label>
  </div>
);

const AudioFile: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [filterType, setFilterType] = useState<BiquadFilterType>('allpass');
  const [filterFreq, setFilterFreq] = useState(350);
  const [filterQ, setFilterQ] = useState(1);
  const [filterGain, setFilterGain] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    const init = async () => {
      const ctx = new AudioContext();
      audioContextRef.current = ctx;

      const filter = ctx.createBiquadFilter();
      filter.connect(ctx.destination);
      filterRef.current = filter;

      setIsLoading(true);
      audioBufferRef.current = await fetch('https://software-mansion.github.io/react-native-audio-api/audio/voice/example-voice-01.mp3')
        .then((response) => response.arrayBuffer())
        .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer))
        .catch((error) => {
          console.error('Error decoding audio data source:', error);
          return null;
        });
      setIsLoading(false);
    };

    init();
    return () => { audioContextRef.current?.close(); };
  }, []);

  useEffect(() => {
    if (!filterRef.current) return;

    const filter = filterRef.current;
    filter.type = filterType;
    filter.frequency.value = filterFreq;
    filter.Q.value = filterQ;
    filter.gain.value = filterGain;

    drawFrequencyResponse();
  }, [filterType, filterFreq, filterQ, filterGain]);

  const handlePlayPause = async () => {
    if (!audioContextRef.current || !audioBufferRef.current) return;

    if (isPlaying) {
      bufferSourceRef.current?.stop();
      bufferSourceRef.current = null;
    } else {
      const source = await audioContextRef.current.createBufferSource();
      source.buffer = audioBufferRef.current;
      bufferSourceRef.current = source;

      source.connect(filterRef.current!);
      source.start();
    }

    setIsPlaying(prev => !prev);
  };

  const drawFrequencyResponse = () => {
    if (!canvasRef.current || !filterRef.current) return;

    const ctx = canvasRef.current.getContext('2d');
    if (!ctx) return;

    const { width, height } = canvasRef.current;

    const N = 200;
    const frequencies = new Float32Array(N).map((_, i) => 20 * 1000 ** (i / (N - 1)));
    const mags = new Float32Array(N);
    const phases = new Float32Array(N);
    filterRef.current.getFrequencyResponse(frequencies, mags, phases);

    // grid
    const line = (x1: number, y1: number, x2: number, y2: number) => {
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
    };

    ctx.clearRect(0, 0, width, height);
    ctx.strokeStyle = '#bcdae4ff';
    ctx.lineWidth = 1;

    const numGridLines = 10;
    for (let i = 0; i <= numGridLines; i++) {
      const x = (i / numGridLines) * width;
      line(x, 0, x, height);
    }

    const dBMarks = [-30, -20, -10, 0, 10, 20, 30];
    for (let i = 0; i < dBMarks.length; i++) {
      const y = (i / (dBMarks.length - 1)) * height;
      line(0, y, width, y);
    }

    // response
    ctx.beginPath();
    mags.forEach((m, i) => {
      const x = (i / (mags.length - 1)) * width;
      const db = 20 * Math.log10(m);
      const y = height - ((db + 30) / 60) * height;
      i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
    });

    ctx.strokeStyle = '#38acdd';
    ctx.lineWidth = 2;
    ctx.stroke();
  };

  return (
    <View style={{ flex: 1, padding: 16 }}>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}

      <canvas ref={canvasRef} width={600} height={200} style={{ borderRadius: 6, marginTop: 16 }} />
      <Button onPress={handlePlayPause} title={isPlaying ? 'Pause' : 'Play'} color="#38acdd" />

      <ScrollView horizontal style={{ marginTop: 16 }}>
        {FILTER_TYPES.map(type => (
          <Pressable
            key={type}
            onPress={() => setFilterType(type)}
            style={[styles.filterButton, filterType === type && { backgroundColor: '#38acdd' }]}
          >
            <Text style={{ color: '#fff' }}>{type}</Text>
          </Pressable>
        ))}
      </ScrollView>

      <div style={{ marginTop: 16 }}>
        <RangeSlider label="Frequency" value={filterFreq} min={10} max={5000} step={10} unit="Hz" onChange={setFilterFreq} />
        <RangeSlider label="Q" value={filterQ} min={0.1} max={20} step={0.1} onChange={setFilterQ} />
        {(filterType === 'peaking' || filterType === 'lowshelf' || filterType === 'highshelf') && (
          <RangeSlider label="Gain" value={filterGain} min={-40} max={40} step={0.5} unit="dB" onChange={setFilterGain} />
        )}
      </div>
    </View>
  );
};

export default AudioFile;

const styles = StyleSheet.create({
  filterButton: {
    paddingVertical: 8,
    paddingHorizontal: 12,
    backgroundColor: '#33488e',
    borderRadius: 6,
    marginRight: 8,
  },
});

// TODO: fix styling in bright mode
