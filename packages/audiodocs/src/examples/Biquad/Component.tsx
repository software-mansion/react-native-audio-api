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

const FILTER_TYPES: (BiquadFilterType | 'none')[] = [
  'none',
  'lowpass',
  'highpass',
  'bandpass',
  'lowshelf',
  'highshelf',
  'peaking',
  'notch',
  'allpass',
];

const AudioFile: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [filterType, setFilterType] = useState<BiquadFilterType | 'none'>('none');
  const [filterFreq, setFilterFreq] = useState(350);
  const [filterQ, setFilterQ] = useState(1);
  const [filterGain, setFilterGain] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  useEffect(() => {
    if (!audioContextRef.current) audioContextRef.current = new AudioContext();

    if (!filterRef.current) {
      filterRef.current = audioContextRef.current.createBiquadFilter();
      filterRef.current.connect(audioContextRef.current.destination);
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      const response = await fetch('/react-native-audio-api/audio/voice/example-voice-01.mp3');
      const arrayBuffer = await response.arrayBuffer();
      audioBufferRef.current = await audioContextRef.current!.decodeAudioData(arrayBuffer);
      setIsLoading(false);
    };

    fetchBuffer();

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  useEffect(() => {
    if (!filterRef.current) return;
    filterRef.current.type = filterType !== 'none'
      ? (filterType as BiquadFilterType)
      : 'allpass';
  }, [filterType]);

  useEffect(() => {
    if (!filterRef.current) return;
    filterRef.current.frequency.value = filterFreq;
    filterRef.current.Q.value = filterQ;
    filterRef.current.gain.value = filterGain;
  }, [filterFreq, filterQ, filterGain]);

  useEffect(() => {
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

      if (filterType !== 'none') {
        source.connect(filterRef.current!);
        filterRef.current?.connect(audioContextRef.current.destination);
      } else {
        source.connect(audioContextRef.current.destination);
      }

      source.start();
    }

    setIsPlaying(prev => !prev);
  };

  const drawFrequencyResponse = () => {
    if (!canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');

    if (!ctx) return;

    const frequencies = new Float32Array(200);
    const magResponse = new Float32Array(frequencies.length);
    const phaseResponse = new Float32Array(frequencies.length);

    for (let i = 0; i < frequencies.length; i++) {
      frequencies[i] = 20 * (20000 / 20) ** (i / (frequencies.length - 1));
    }

    if (filterType !== 'none' && filterRef.current) {
      filterRef.current.getFrequencyResponse(frequencies, magResponse, phaseResponse);
    } else {
      magResponse.fill(1);
    }

    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.fillStyle = '#111';
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.strokeStyle = '#444';
    ctx.lineWidth = 1;

    const numGridLines = 10;
    const gridSpacing = canvas.width / numGridLines;
    for (let i = 0; i <= numGridLines; i++) {
      const x = i * gridSpacing;
      ctx.beginPath();
      ctx.moveTo(x, 0);
      ctx.lineTo(x, canvas.height);
      ctx.stroke();
    }

    const dBMarks = [-30, -20, -10, 0, 10, 20, 30];
    const dBSpacing = canvas.height / (dBMarks.length - 1);
    for (let i = 0; i < dBMarks.length; i++) {
      const y = i * dBSpacing;
      ctx.beginPath();
      ctx.moveTo(0, y);
      ctx.lineTo(canvas.width, y);
      ctx.stroke();
    }

    ctx.beginPath();
    for (let i = 0; i < frequencies.length; i++) {
      const x = (i / (frequencies.length - 1)) * canvas.width;
      const db = 20 * Math.log10(magResponse[i]);
      const y = canvas.height - ((db + 30) / 60) * canvas.height;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.strokeStyle = '#38acdd';
    ctx.lineWidth = 2;
    ctx.stroke();
  };

  return (
    <View style={{ flex: 1, padding: 16 }}>
      {isLoading && <ActivityIndicator color="#FFFFFF" />}

      <canvas ref={canvasRef} width={600} height={200} style={{ backgroundColor: '#111', borderRadius: 6, marginTop: 16 }} />
      <Button onPress={handlePlayPause} title={isPlaying ? 'Pause' : 'Play'} color="#38acdd" />

      <ScrollView horizontal style={{ marginTop: 16 }}>
        {FILTER_TYPES.map(type => (
          <Pressable
            key={type}
            onPress={() => setFilterType(type)}
            style={[
              styles.filterButton,
              filterType === type && { backgroundColor: '#38acdd' },
            ]}
          >
            <Text style={{ color: '#fff' }}>{type}</Text>
          </Pressable>
        ))}
      </ScrollView>

      {filterType !== 'none' && (
        <div style={{ marginTop: 16 }}>
          <label style={{ color: '#fff' }}>
            Frequency: {filterFreq.toFixed(0)} Hz
            <input
              type="range"
              min={10}
              max={5000}
              step={10}
              value={filterFreq}
              onChange={(e) => setFilterFreq(Number(e.target.value))}
              style={{ width: '100%', marginTop: 4 }}
            />
          </label>

          <label style={{ color: '#fff', marginTop: 8, display: 'block' }}>
            Q: {filterQ.toFixed(2)}
            <input
              type="range"
              min={0.1}
              max={20}
              step={0.1}
              value={filterQ}
              onChange={(e) => setFilterQ(Number(e.target.value))}
              style={{ width: '100%', marginTop: 4 }}
            />
          </label>

          {(filterType === 'peaking' || filterType === 'lowshelf' || filterType === 'highshelf') && (
            <label style={{ color: '#fff', marginTop: 8, display: 'block' }}>
              Gain: {filterGain.toFixed(1)} dB
              <input
                type="range"
                min={-40}
                max={40}
                step={0.5}
                value={filterGain}
                onChange={(e) => setFilterGain(Number(e.target.value))}
                style={{ width: '100%', marginTop: 4 }}
              />
            </label>
          )}
        </div>
      )}
    </View>
  );
};

export default AudioFile;

const styles = StyleSheet.create({
  filterButton: {
    paddingVertical: 8,
    paddingHorizontal: 12,
    backgroundColor: '#333',
    borderRadius: 6,
    marginRight: 8,
  },
});

// TODO:
// fix styling
// case when none is selected
