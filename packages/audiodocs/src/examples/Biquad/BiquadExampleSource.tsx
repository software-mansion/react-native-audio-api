import React, { useState, useEffect, useRef } from 'react';
import { View, ActivityIndicator } from 'react-native';
import Animated, { useSharedValue, useAnimatedStyle, withTiming } from 'react-native-reanimated';
import { AudioContext, AudioBufferSourceNode, BiquadFilterNode, GainNode } from 'react-native-audio-api';

const AudioFile: React.FC = () => {
  const [loading, setLoading] = useState(false);
  const [slider, setSlider] = useState(0);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const filterRef = useRef<BiquadFilterNode | null>(null);
  const gainRef = useRef<GainNode | null>(null);

  const squareX = useSharedValue(0);

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

      setLoading(true);
      audioBufferRef.current = await fetch('/react-native-audio-api/audio/music/example-music-01.mp3')
        .then((response) => response.arrayBuffer())
        .then((arrayBuffer) => ctx.decodeAudioData(arrayBuffer))
        .catch((error) => {
          console.error('Error decoding audio data source:', error);
          return null;
        });
      setLoading(false);
    };

    init();
    return () => {
      audioContextRef.current?.close();
    };
  }, []);


  const startSound = async () => {
    if (!audioContextRef.current || !audioBufferRef.current) return;
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

  const animatedStyle = useAnimatedStyle(() => ({
    transform: [{ translateX: withTiming(slider / 100 * 120, { duration: 50 }) }],
  }));

  useEffect(() => {
    if (filterRef.current && gainRef.current) {
      const ratio = slider / 100;
      filterRef.current.frequency.value = 200 + ratio * (5000 - 200);
      filterRef.current.Q.value = 1 + (1 - ratio) * 10;
      gainRef.current.gain.value = 0.2 + ratio * 0.8;
    }
  }, [slider]);

  return (
    <View style={{ flex: 1, alignItems: 'center', padding: 16 }}>
      {loading && <ActivityIndicator color="#FFF" />}

      <View style={{ position: 'absolute', top: 150, width: 200, height: 100, alignItems: 'center' }}>
        <View style={{ width: 80, height: 80, borderRadius: 40, backgroundColor: '#ff5722', position: 'absolute', left: 0, top: 10 }} />
        <Animated.View style={[{ width: 80, height: 80, backgroundColor: 'rgba(0,0,0,0.8)', position: 'absolute', top: 10 }, animatedStyle]} />
      </View>

      <div style={{ marginTop: 300, width: 180 }}>
        <input
          type="range"
          min={0}
          max={100}
          value={slider}
          onChange={e => setSlider(Number(e.target.value))}
          onMouseDown={startSound}
          onMouseUp={stopSound}
          onTouchStart={startSound}
          onTouchEnd={stopSound}
          style={{ width: '100%', marginTop: 20 }}
        />
      </div>
    </View>
  );
};

export default AudioFile;
