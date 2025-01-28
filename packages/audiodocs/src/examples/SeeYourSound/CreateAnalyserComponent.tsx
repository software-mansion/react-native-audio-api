import React, {
  useState,
  useEffect,
  useRef,
  useContext,
  createContext
} from 'react';
import {
  AudioContext,
  AudioBuffer,
  AudioBufferSourceNode,
  AnalyserNode,
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

interface ChartProps {
  data: number[];
  size: number;
}

const TimeChart: React.FC<ChartProps> = (props) => {
  const size = useContext(CanvasContext);
}

const FFT_SIZE = 512;

const AudioVisualizer: React.FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);
  const [isLoading, setIsLoading] = useState(false);
  const [times, setTimes] = useState<number[]>(new Array(FFT_SIZE).fill(127));
  const [freqs, setFreqs] = useState<number[]>(new Array(FFT_SIZE / 2).fill(0));

  const audioContextRef = useRef<AudioContext | null>(null);
  const bufferSourceRef = useRef<AudioBufferSourceNode | null>(null);
  const audioBufferRef = useRef<AudioBuffer | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);


  const handlePlayPause = () => {
    if (isPlaying) {
      bufferSourceRef.current?.stop();
    } else {
      if (!audioContextRef.current || !analyserRef.current) {
        return
      }

      bufferSourceRef.current = audioContextRef.current.createBufferSource();
      bufferSourceRef.current.buffer = audioBufferRef.current;
      bufferSourceRef.current.connect(analyserRef.current);

      bufferSourceRef.current.start();

      requestAnimationFrame(draw);
    }

    setIsPlaying((prev) => !prev);
  };

  const draw = () => {
    if (!analyserRef.current) {
      return;
    }

    const timesArrayLength = analyserRef.current.fftSize;
    const frequencyArrayLength = analyserRef.current.frequencyBinCount;

    const timesArray = new Array(timesArrayLength);
    analyserRef.current.getByteTimeDomainData(timesArray);
    setTimes(timesArray);

    const freqsArray = new Array(frequencyArrayLength);
    analyserRef.current.getByteFrequencyData(freqsArray);
    setFreqs(freqsArray);

    requestAnimationFrame(draw);
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    if (!analyserRef.current) {
      analyserRef.current = audioContextRef.current.createAnalyser();
      analyserRef.current.fftSize = FFT_SIZE;
      analyserRef.current.smoothingTimeConstant = 0.8;

      analyserRef.current.connect(audioContextRef.current.destination);
    }

    const fetchBuffer = async () => {
      setIsLoading(true);
      audioBufferRef.current = await audioContextRef.current!.decodeAudioDataSource('/react-native-audio-api/audio/music/example-music-02.mp3');

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

