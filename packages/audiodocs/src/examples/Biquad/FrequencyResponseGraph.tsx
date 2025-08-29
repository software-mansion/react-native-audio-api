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
} from 'react-native';
import { drawFrequencyResponse } from './drawFrequencyResponse';
import RangeSlider from './RangeSlider';

const FILTER_TYPES: BiquadFilterType[] = [
  'allpass', 'lowpass', 'highpass', 'bandpass',
  'lowshelf', 'highshelf', 'peaking', 'notch',
];


const FrequencyResponseGraph: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [filterType, setFilterType] = useState<BiquadFilterType>('allpass');
  const [filterFreq, setFilterFreq] = useState(350);
  const [filterQ, setFilterQ] = useState(1);
  const [filterGain, setFilterGain] = useState(0);
  const [selectedAudio, setSelectedAudio] = useState<'speech' | 'music'>('speech');

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBuffersRef = useRef<{ speech: AudioBuffer | null; music: AudioBuffer | null }>({
    speech: null,
    music: null,
  });
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
      try {
        const speech = await fetch('/react-native-audio-api/audio/voice/example-voice-01.mp3')
          .then((response) => response.arrayBuffer())
          .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer));

        const music = await fetch('/react-native-audio-api/audio/music/example-music-03.mp3')
          .then((response) => response.arrayBuffer())
          .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer));

        audioBuffersRef.current = { speech, music };
      } catch (error) {
        console.error('Error decoding audio data source:', error);
      }
      setIsLoading(false);
    };

    init();
    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  useEffect(() => {
    if (!filterRef.current) return;

    const filter = filterRef.current;
    filter.type = filterType;
    filter.frequency.value = filterFreq;
    filter.Q.value = filterQ;
    filter.gain.value = filterGain;

    drawFrequencyResponse(canvasRef.current, filterRef.current);
  }, [filterType, filterFreq, filterQ, filterGain]);

  const playAudio = async () => {
    if (!audioContextRef.current) return;

    bufferSourceRef.current?.stop();
    bufferSourceRef.current = null;

    const buffer = selectedAudio === 'speech' ? audioBuffersRef.current.speech : audioBuffersRef.current.music;
    if (!buffer) return;

    const source = await audioContextRef.current.createBufferSource();
    source.buffer = buffer;
    bufferSourceRef.current = source;

    source.connect(filterRef.current!);
    source.start();

    setIsPlaying(true);
  };

  const handlePlayPause = async () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
      bufferSourceRef.current = null;
      setIsPlaying(false);
    } else {
      await playAudio();
    }
  };

  useEffect(() => {
    if (isPlaying) playAudio();
  }, [selectedAudio]);

  return (
    <View style={styles.container}>
      {isLoading && <ActivityIndicator color="#fcfcff" />}

      <canvas ref={canvasRef} width={600} height={200} style={styles.canvas} />
      <Button onPress={handlePlayPause} title={isPlaying ? 'Pause' : 'Play'} color="#33488e" />

      <View style={[styles.filterButtonsContainer, { marginTop: 16 }]}>
        {FILTER_TYPES.map((type) => (
          <Pressable
            key={type}
            onPress={() => setFilterType(type)}
            style={[
              styles.filterButton,
              filterType === type && styles.filterButtonActive,
            ]}
          >
            <Text style={styles.labelText}>{type}</Text>
          </Pressable>
        ))}
      </View>

      <View style={styles.audioToggleContainer}>
        <Text style={[styles.labelText, { marginRight: 8 }]}>Speech</Text>
        <Pressable
          onPress={() => setSelectedAudio(selectedAudio === 'speech' ? 'music' : 'speech')}
          style={styles.toggleTrack}
        >
          <View
            style={[
              styles.toggleThumb,
              { transform: [{ translateX: selectedAudio === 'speech' ? 0 : 26 }] },
            ]}
          />
        </Pressable>
        <Text style={[styles.labelText, { marginLeft: 8 }]}>Music</Text>
      </View>

      <div style={{ marginTop: 16 }}>
        <RangeSlider label="Frequency" value={filterFreq} min={10} max={5000} step={10} unit="Hz" onChange={setFilterFreq} />
        {(filterType !== 'lowshelf' && filterType !== 'highshelf') && (
          <RangeSlider label="Q" value={filterQ} min={0.1} max={20} step={0.1} onChange={setFilterQ} />
        )}
        {(filterType === 'peaking' || filterType === 'lowshelf' || filterType === 'highshelf') && (
          <RangeSlider label="Gain" value={filterGain} min={-40} max={40} step={0.5} unit="dB" onChange={setFilterGain} />
        )}
      </div>
    </View>
  );
};

export default FrequencyResponseGraph;

const styles = StyleSheet.create({
  container: { flex: 1, padding: 16 },
  canvas: { borderRadius: 6, marginTop: 16 },
  filterButton: {
    paddingVertical: 8,
    paddingHorizontal: 12,
    backgroundColor: '#6676aa',
    borderRadius: 6,
    marginRight: 8,
  },
  filterButtonsContainer: {
    flexDirection: 'row',
    flexWrap: 'wrap',
  },
  filterButtonActive: {
    backgroundColor: '#33488e',
  },
  labelText: {
    color: '#fcfcff',
  },
  audioToggleContainer: {
    flexDirection: 'row',
    alignItems: 'center',
    marginTop: 16,
  },
  toggleTrack: {
    width: 50,
    height: 24,
    borderRadius: 12,
    backgroundColor: '#33488e',
    padding: 2,
    justifyContent: 'center',
  },
  toggleThumb: {
    width: 20,
    height: 20,
    borderRadius: 10,
    backgroundColor: '#6676aa',
  },
});
