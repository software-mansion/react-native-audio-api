import React, {
  useState,
  useEffect,
  useRef,
  useContext,
  useMemo,
  createContext,
  PropsWithChildren
} from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button, LayoutChangeEvent } from 'react-native';
import { Canvas as SKCanvas } from '@shopify/react-native-skia';

interface Size {
  width: number;
  height: number;
}

interface CanvasContext {
  initialized: boolean;
  size: Size;
}

const CanvasContext = createContext<CanvasContext>({
  initialized: false,
  size: { width: 0, height: 0 },
});

const Canvas: React.FC<PropsWithChildren> = ({ children }) => {
  const [size, setSize] = useState<Size>({ width: 0, height: 0 });

  const onCanvasLayout = (event: LayoutChangeEvent) => {
    const { width, height } = event.nativeEvent.layout;

    setSize({ width, height });
  };

  const context = useMemo(
    () => ({
      initialized: true,
      size: { width: size.width, height: size.height },
    }),
    [size.width, size.height]
  );

  return (
    <SKCanvas style={{flex: 1}} onLayout={onCanvasLayout}>
      <CanvasContext.Provider value={context}>
        {children}
      </CanvasContext.Provider>
    </SKCanvas>
  );
};

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);

  const handlePlayPause = () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current) {
        return
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBufferRef.current;
      bufferSourceRef.current.connect(audioContextRef.current.destination);

      bufferSourceRef.current.start();
    }

    setIsPlaying((prev) => !prev);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      audioBufferRef.current = await fetch('https://software-mansion-labs.github.io/react-native-audio-api/audio/music/example-music-02.mp3')
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
      <Canvas>
      </Canvas>
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
          />
        </View>
      </View>
    </View>
  );
};

export default AudioVisualizer;

