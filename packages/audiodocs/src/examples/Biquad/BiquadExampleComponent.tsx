import React, { useState, useEffect, useRef } from 'react';
import { View, ViewStyle, Image } from "react-native";
import Animated, { useAnimatedStyle, withTiming } from "react-native-reanimated";
import { AudioContext, AudioBufferSourceNode, BiquadFilterNode, GainNode, AudioBuffer } from 'react-native-audio-api';

type FlapParams = {
  axis: "X" | "Y";
  translate: number;
  multiplier: number;
};

const useFlapStyle = (progress: number, { axis, translate, multiplier }: FlapParams) => {
  return useAnimatedStyle(() => {
    const rotate = `${progress * multiplier}deg`;
    return {
      transform: [
        { perspective: 800 },
        axis === "X" ? { translateY: translate } : { translateX: translate },
        axis === "X"
          ? { rotateX: withTiming(rotate, { duration: 50 }) }
          : { rotateY: withTiming(rotate, { duration: 50 }) },
        axis === "X" ? { translateY: -translate } : { translateX: -translate },
      ],
    };
  });
};

const BoxWithFlaps = ({ progress }: { progress: number }) => {
  const topStyle = useFlapStyle(progress, { axis: "X", translate: -50, multiplier: -120 });
  const bottomStyle = useFlapStyle(progress, { axis: "X", translate: 50, multiplier: 120 });
  const leftStyle = useFlapStyle(progress, { axis: "Y", translate: -50, multiplier: -110 });
  const rightStyle = useFlapStyle(progress, { axis: "Y", translate: 50, multiplier: 110 });

  const baseFlapStyle: ViewStyle = {
    position: "absolute",
    backgroundColor: "#a9744f",
    borderColor: "#5c3d2e",
  };

  const flapConfigs = [
    {
      key: "top",
      style: { top: 0, left: 1, width: 198, height: 100, borderBottomWidth: 2 },
      animatedStyle: topStyle,
    },
    {
      key: "bottom",
      style: { bottom: 0, left: 1, width: 198, height: 100, borderTopWidth: 2 },
      animatedStyle: bottomStyle,
    },
    {
      key: "left",
      style: { top: 0, left: 0, width: 100, height: 200, borderRightWidth: 2, zIndex: 1 },
      animatedStyle: leftStyle,
    },
    {
      key: "right",
      style: { top: 0, right: 0, width: 100, height: 200, borderLeftWidth: 2, zIndex: 1 },
      animatedStyle: rightStyle,
    },
  ];

  return (
    <View style={{ width: 200, height: 200, position: "relative", alignItems: "center", justifyContent: "center" }}>
      {/* Box base */}
      <View
        style={{
          position: "absolute",
          width: 200,
          height: 200,
          backgroundColor: "#9d6c49ff",
          borderWidth: 2,
          borderColor: "#5c3d2e",
          borderRadius: 4,
        }}
      />

      {/* Gramophone */}
      <Image
        style={{ width: 100, height: 100, position: "absolute" }}
        source={{
          uri: "https://img.freepik.com/premium-vector/gramophone-illustration-vector-music-cartoon_773815-277.jpg",
        }}
      />

      {/* Flaps */}
      {flapConfigs.map((flap) => (
        <Animated.View
          key={flap.key}
          style={[baseFlapStyle, flap.style, flap.animatedStyle]}
        />
      ))}
    </View>
  );
};

const AudioFile: React.FC = () => {
  const [loading, setLoading] = useState(false);
  const [slider, setSlider] = useState(0);

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
    if( bufferSourceRef.current != null ) {
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
  }, [slider]);

  return (
    <View style={{ flex: 1, alignItems: "center", justifyContent: "center" }}>
      <BoxWithFlaps progress={slider / 100} />
      <div style={{ marginTop: 50, width: 180 }}>
        <input
          type="range"
          min={0}
          max={100}
          value={slider}
          onChange={(e) => setSlider(Number(e.target.value))}
          onMouseDown={startSound}
          onMouseUp={stopSound}
          onTouchStart={startSound}
          onTouchEnd={stopSound}
          style={{ width: "100%", marginTop: 20 }}
        />
      </div>
    </View>
  );
};

export default AudioFile;
