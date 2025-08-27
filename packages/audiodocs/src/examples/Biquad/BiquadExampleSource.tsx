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
  const phaseShift = 0.7;
  const delayedProgress = Math.max(0, progress - phaseShift) / (1 - phaseShift);

  const topStyle = useFlapStyle(delayedProgress, { axis: "X", translate: -25, multiplier: 130 });
  const bottomStyle = useFlapStyle(delayedProgress, { axis: "X", translate: 25, multiplier: -130 });
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
      style: { top: 0, left: 1, width: 198, height: 50, borderBottomWidth: 2 },
      animatedStyle: topStyle,
    },
    {
      key: "bottom",
      style: { bottom: 0, left: 1, width: 198, height: 50, borderTopWidth: 2 },
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
        style={{ width: 250, height: 250, position: "absolute" }}
        source={{
          uri: 'https://sdmntprsouthcentralus.oaiusercontent.com/files/00000000-b3fc-61f7-85f7-0df1cc60ecca/raw?se=2025-08-27T16%3A46%3A06Z&sp=r&sv=2024-08-04&sr=b&scid=4ab0bd70-aeeb-5211-ab2d-533022fdef61&skoid=eb780365-537d-4279-a878-cae64e33aa9c&sktid=a48cca56-e6da-484e-a814-9c849652bcb3&skt=2025-08-27T04%3A51%3A33Z&ske=2025-08-28T04%3A51%3A33Z&sks=b&skv=2024-08-04&sig=sRrtIPMqvZzo0sjZ0eU1ybRDxjfC9SfpdJoliI8av4s%3D',
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
      const id = requestAnimationFrame(() => setSlider(prev => Math.max(0, prev - 1)));
      return () => cancelAnimationFrame(id);
    }
  }, [slider, sliderPressed]);

  return (
    <View style={{ flex: 1, alignItems: "center", justifyContent: "center" }}>
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

export default AudioFile;
