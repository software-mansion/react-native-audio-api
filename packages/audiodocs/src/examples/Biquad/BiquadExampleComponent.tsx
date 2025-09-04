import React, { useState, useEffect, useRef } from 'react';
import { View } from 'react-native';
import {
  AudioContext,
  AudioBufferSourceNode,
  BiquadFilterNode,
  GainNode,
  AudioBuffer,
} from 'react-native-audio-api';
import { BoxWithFlaps } from './BoxWithFlaps';

const BiquadExample: React.FC = () => {
  const [slider, setSlider] = useState(0);
  const [sliderPressed, setSliderPressed] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);

  useEffect(() => {
    const init = async () => {
      const ctx = new AudioContext();
      audioContextRef.current = ctx;

      const filterNode = ctx.createBiquadFilter();
      filterNode.type = 'lowpass';
      const gainNode = ctx.createGain();
      filterNode.connect(gainNode);
      gainNode.connect(ctx.destination);

      filterRef.current = filterNode;
      gainRef.current = gainNode;

      audioBufferRef.current = await fetch('/react-native-audio-api/audio/music/example-music-01.mp3')
        .then((response) => response.arrayBuffer())
        .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer))
        .catch((error) => {
          console.error('Error decoding audio data source:', error);
          return null;
        });
    };

    init();
    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  const playSound = async () => {
    if (!audioContextRef.current || !audioBufferRef.current) return;
    if (bufferSourceRef.current != null) {
      stopSound();
    }
    const source = await audioContextRef.current.createBufferSource();
    source.buffer = audioBufferRef.current;
    bufferSourceRef.current = source;

    source.connect(filterRef.current!);
    source.start();
  };

  const stopSound = () => {
    bufferSourceRef.current?.stop();
    bufferSourceRef.current = null;
  };

  useEffect(() => {
    if (filterRef.current && gainRef.current) {
      const ratio = slider / 100;
      filterRef.current.frequency.value = 200 + ratio * (5000 - 200);
      filterRef.current.Q.value = 1 + (1 - ratio) * 10;
      gainRef.current.gain.value = 0.2 + ratio * 0.8;
    }

    if ((sliderPressed || slider > 0) && !bufferSourceRef.current) playSound();
    if (!sliderPressed && slider === 0) stopSound();

    if (!sliderPressed && slider > 0) {
      const id = requestAnimationFrame(() =>
        setSlider((prev) => Math.max(0, prev - 1))
      );
      return () => cancelAnimationFrame(id);
    }
  }, [slider, sliderPressed]);

  return (
    <View style={{ flex: 1, alignItems: 'center', justifyContent: 'center' }}>
      <BoxWithFlaps progress={slider / 100} />
      <input
        type="range"
        min={0}
        max={100}
        value={slider}
        onChange={(e) => setSlider(Number(e.target.value))}
        onMouseDown={() => setSliderPressed(true)}
        onMouseUp={() => setSliderPressed(false)}
        onTouchStart={() => setSliderPressed(true)}
        onTouchEnd={() => setSliderPressed(false)}
        style={{ width: 200, marginTop: 50 }}
      />
    </View>
  );
};

export default BiquadExample;
