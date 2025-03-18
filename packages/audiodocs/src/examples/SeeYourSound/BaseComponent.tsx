import React, {
  useState,
  useEffect,
  useRef,
  createContext
} from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
} from 'react-native-audio-api';
import { ActivityIndicator, View, Button } from 'react-native';

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

const Canvas: React.FC<React.PropsWithChildren<{}>> = ({ children }) => {
  const [size, setSize] = useState<Size>({ width: 0, height: 0 });
  const ref = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    const handleResize = () => {
      if (ref.current) {
        const { width, height } = ref.current.getBoundingClientRect();
        setSize({ width, height });
      }
    };

    window.addEventListener('resize', handleResize);
    handleResize();

    return () => window.removeEventListener('resize', handleResize);
  }, [ref]);

  const contextValue = { initialized: true, size };

  return (
    <canvas ref={ref} style={{ backgroundColor: 'white', width: '100%', height: '100%' }}>
      <CanvasContext.Provider value={contextValue}>
        {children}
      </CanvasContext.Provider>
    </canvas>
  );
};

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);

  const handlePlayPause = async () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current) {
        return
      }

      bufferSourceRef.current = await audioContextRef.current.createBufferSource();
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

