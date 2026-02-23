import { useColorMode } from '@docusaurus/theme-common';
import React, { useCallback, useEffect, useRef, useState } from 'react';
import {
  AudioBuffer,
  AudioBufferSourceNode,
  AudioContext,
  BiquadFilterNode,
} from 'react-native-audio-api';

import DetailBox from '@site/src/ui/DetailBox';
import FilterList from '@site/src/ui/FilterList';
import type { ResponsiveCanvasDrawParams } from '@site/src/ui/ResponsiveCanvas';
import ResponsiveCanvas from '@site/src/ui/ResponsiveCanvas';
import SliderInput from '@site/src/ui/SliderInput';
import Switch from '@site/src/ui/Switch';
import { drawFrequencyResponse } from './drawFrequencyResponse';
import styles from './FrequencyResponseGraph.module.css';

const FILTER_OPTIONS: Array<{ value: BiquadFilterType; label: string }> = [
  { value: 'allpass', label: 'All-pass' },
  { value: 'lowpass', label: 'Low-pass' },
  { value: 'highpass', label: 'High-pass' },
  { value: 'bandpass', label: 'Band-pass' },
  { value: 'lowshelf', label: 'Low-shelf' },
  { value: 'highshelf', label: 'High-shelf' },
  { value: 'peaking', label: 'Peaking' },
  { value: 'notch', label: 'Notch' },
];

const FrequencyResponseGraph: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [filterType, setFilterType] = useState<BiquadFilterType>('allpass');
  const [filterFreq, setFilterFreq] = useState(350);
  const [filterQ, setFilterQ] = useState(1);
  const [filterGain, setFilterGain] = useState(0);
  const [selectedAudio, setSelectedAudio] = useState<'speech' | 'music'>('speech');
  const [isFilterReady, setIsFilterReady] = useState(false);
  const { colorMode } = useColorMode();

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBuffersRef = useRef<{ speech: AudioBuffer | null; music: AudioBuffer | null }>({
    speech: null,
    music: null,
  });
  const filterRef = useRef<BiquadFilterNode | null>(null);

  const gridColor = colorMode === 'dark' ? 'rgba(252, 252, 255, 0.15)' : 'rgba(0, 0, 0, 0.2)';
  const isSpeech = selectedAudio === 'speech';

  useEffect(() => {
    let mounted = true;

    const init = async () => {
      const ctx = new AudioContext();
      audioContextRef.current = ctx;

      const filter = ctx.createBiquadFilter();
      filter.connect(ctx.destination);
      filterRef.current = filter;
      setIsFilterReady(true);

      setIsLoading(true);
      try {
        const speech = await fetch('/react-native-audio-api/audio/voice/example-voice-01.mp3')
          .then((response) => response.arrayBuffer())
          .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer));

        const music = await fetch('/react-native-audio-api/audio/music/example-music-03.mp3')
          .then((response) => response.arrayBuffer())
          .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer));

        if (mounted) {
          audioBuffersRef.current = { speech, music };
        }
      } catch (error) {
        console.error('Error decoding audio data source:', error);
      } finally {
        if (mounted) {
          setIsLoading(false);
        }
      }
    };

    init();

    return () => {
      mounted = false;
      bufferSourceRef.current?.stop();
      bufferSourceRef.current = null;
      audioContextRef.current?.close();
    };
  }, []);

  const handleCanvasDraw = useCallback(({ canvas }: ResponsiveCanvasDrawParams) => {
    if (!isFilterReady || !filterRef.current) {
      return;
    }

    const filter = filterRef.current;
    filter.type = filterType;
    filter.frequency.value = filterFreq;
    filter.Q.value = filterQ;
    filter.gain.value = filterGain;

    drawFrequencyResponse(canvas, filter, gridColor);
  }, [filterFreq, filterGain, filterQ, filterType, gridColor, isFilterReady]);

  const playAudio = async () => {
    const context = audioContextRef.current;
    if (!context) {
      return;
    }

    bufferSourceRef.current?.stop();
    bufferSourceRef.current = null;

    const buffer = selectedAudio === 'speech'
      ? audioBuffersRef.current.speech
      : audioBuffersRef.current.music;

    if (!buffer) {
      return;
    }

    const source = await context.createBufferSource();
    source.buffer = buffer;
    bufferSourceRef.current = source;
    source.connect(filterRef.current!);
    source.onEnded = () => {
      if (bufferSourceRef.current === source) {
        bufferSourceRef.current = null;
        setIsPlaying(false);
      }
    };
    source.start();
    setIsPlaying(true);
  };

  const handlePlayPause = async () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
      bufferSourceRef.current = null;
      setIsPlaying(false);
      return;
    }

    await playAudio();
  };

  useEffect(() => {
    if (isPlaying) {
      void playAudio();
    }
  }, [selectedAudio]);

  return (
    <DetailBox tag="BiquadFilterNode" info="interactive playground" startOpen>
      <div className={styles.container}>
        {isLoading && (
          <div className={styles.loadingRow}>
            <span className={styles.loadingSpinner} aria-hidden="true" />
            <span className={styles.loadingText}>Loading audio...</span>
          </div>
        )}

        <ResponsiveCanvas
          onDraw={handleCanvasDraw}
          aspectRatio={3}
          containerClassName={styles.canvasContainer}
        />

        <div className={styles.playPauseButtonContainer}>
          <button type="button" onClick={handlePlayPause} className={styles.playPauseButton}>
            {isPlaying ? 'Pause' : 'Play'}
          </button>
        </div>

        <FilterList<BiquadFilterType>
          options={FILTER_OPTIONS}
          value={filterType}
          onChange={setFilterType}
        />

        <Switch
          checked={!isSpeech}
          onChange={(isMusic) => setSelectedAudio(isMusic ? 'music' : 'speech')}
          leftLabel="Speech"
          rightLabel="Music"
          ariaLabel="Toggle between speech and music samples"
        />

        <div className={styles.sliderSection}>
          <SliderInput
            label="Frequency"
            value={filterFreq}
            min={10}
            max={5000}
            step={10}
            unit="Hz"
            onChange={setFilterFreq}
          />

          {filterType !== 'lowshelf' && filterType !== 'highshelf' && (
            <SliderInput
              label="Q"
              value={filterQ}
              min={0.1}
              max={20}
              step={0.1}
              onChange={setFilterQ}
            />
          )}

          {(filterType === 'peaking' || filterType === 'lowshelf' || filterType === 'highshelf') && (
            <SliderInput
              label="Gain"
              value={filterGain}
              min={-40}
              max={40}
              step={0.5}
              unit="dB"
              onChange={setFilterGain}
            />
          )}
        </div>
      </div>
    </DetailBox>
  );
};

export default FrequencyResponseGraph;
