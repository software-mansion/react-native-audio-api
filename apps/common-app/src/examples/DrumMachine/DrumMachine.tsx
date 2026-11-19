import { Canvas, useCanvasRef } from '@shopify/react-native-skia';
import React, {
  useCallback,
  useEffect,
  useLayoutEffect,
  useRef,
  useState,
} from 'react';
import { StyleSheet, Text, View } from 'react-native';
import {
  AudioBuffer,
  AudioContext,
  decodeAudioData,
} from 'react-native-audio-api';
import { GestureDetector } from 'react-native-gesture-handler';
import Animated, {
  SharedValue,
  useAnimatedStyle,
  withTiming,
} from 'react-native-reanimated';

import { Container } from '../../components';
import { useControlsSheet } from '../../navigation/useControlsSheet';
import { colors } from '../../styles';
import type { InstrumentName, PlayingInstruments, XYWHRect } from '../../types';
import Clap from '../../utils/soundEngines/Clap';
import HiHat from '../../utils/soundEngines/HiHat';
import Kick from '../../utils/soundEngines/Kick';
import playSample from '../../utils/soundEngines/playSample';
import usePlayer from '../../utils/usePlayer';
import CenterDisc from './CenterDisc';
import { numBeats, size } from './constants';
import {
  drumMachineStore,
  SoundSource,
  useDrumMachineState,
} from './drumMachineStore';
import Grid from './Grid';
import NotesHighlight from './NotesHighlight';
import PatternShape from './PatternShape';
import hiHatSample from './samples/hi-hat.wav';
import kickSample from './samples/kick.wav';
import snareSample from './samples/snare.wav';
import useGestures from './useGestures';

type DrumVoice = 'kick' | 'clap' | 'hi-hat';
type SampleBuffers = Record<DrumVoice, AudioBuffer>;

/**
 * Acoustic samples per sequencer voice; the `clap` ring plays a snare in sample
 * mode.
 */
const sampleSources: Record<DrumVoice, number> = {
  kick: kickSample,
  clap: snareSample,
  'hi-hat': hiHatSample,
};

function isDrumVoice(name: InstrumentName): name is DrumVoice {
  return name in sampleSources;
}

async function decodeSamples(): Promise<SampleBuffers> {
  const voices = Object.keys(sampleSources) as DrumVoice[];
  const buffers = await Promise.all(
    voices.map((voice) => decodeAudioData(sampleSources[voice]))
  );

  return Object.fromEntries(
    voices.map((voice, index) => [voice, buffers[index]])
  ) as SampleBuffers;
}

interface TempoReadoutProps {
  bpm: number;
  playingInstruments: SharedValue<PlayingInstruments>;
}

const kickPulseTiming = { duration: 25 };

const TempoReadout: React.FC<TempoReadoutProps> = ({
  bpm,
  playingInstruments,
}) => {
  const pulseStyle = useAnimatedStyle(() => ({
    transform: [
      {
        scale: withTiming(
          playingInstruments.value.kick ? 1.08 : 1,
          kickPulseTiming
        ),
      },
    ],
  }));

  return (
    <View style={styles.readout} pointerEvents="none">
      <Animated.View style={[styles.readoutContent, pulseStyle]}>
        <Text style={styles.bpm}>{bpm}</Text>
        <Text style={styles.bpmCaption}>BPM</Text>
      </Animated.View>
    </View>
  );
};

const DrumMachine: React.FC = () => {
  const { bpm, patterns, isPlaying, soundSource, controlsHeight } =
    useDrumMachineState();

  useControlsSheet('DrumMachineControls');

  const soundSourceRef = useRef<SoundSource>(soundSource);
  const sampleBuffersRef = useRef<SampleBuffers | null>(null);

  useLayoutEffect(() => {
    soundSourceRef.current = soundSource;
  }, [soundSource]);

  useEffect(() => {
    let isMounted = true;

    decodeSamples().then((buffers) => {
      if (isMounted) {
        sampleBuffersRef.current = buffers;
      }
    });

    return () => {
      isMounted = false;
      drumMachineStore.reset();
    };
  }, []);

  /**
   * Reads the sound source and the decoded samples through refs, so switching
   * between synth and samples takes effect on the next hit without restarting
   * the loop (`usePlayer` recreates its context whenever `setup` changes).
   */
  const setupPlayer = useCallback((audioContext: AudioContext) => {
    const synth: Record<DrumVoice, { play: (time: number) => void }> = {
      kick: new Kick(audioContext),
      clap: new Clap(audioContext),
      'hi-hat': new HiHat(audioContext),
    };

    const playNote = (name: InstrumentName, time: number) => {
      if (!isDrumVoice(name)) {
        return;
      }

      const samples = sampleBuffersRef.current;
      if (soundSourceRef.current === 'samples' && samples) {
        playSample(audioContext, samples[name], time);
      } else {
        synth[name].play(time);
      }
    };

    return { playNote };
  }, []);

  const player = usePlayer({
    bpm,
    patterns,
    notesPerBeat: 2,
    numBeats,
    setup: setupPlayer,
  });

  const { play, stop } = player;
  useEffect(() => {
    if (isPlaying) {
      play();
    } else {
      stop();
    }
  }, [isPlaying, play, stop]);

  const canvasRef = useCanvasRef();
  const [canvasRect, setCanvasRect] = useState<XYWHRect>({
    x: 0,
    y: 0,
    width: size,
    height: size,
  });

  useLayoutEffect(() => {
    canvasRef.current?.measure((x, y, width, height) => {
      setCanvasRect({ x, y, width, height });
    });
  }, [canvasRef]);

  const gesture = useGestures({
    canvasRect,
    onPatternChange: drumMachineStore.toggleStep,
  });

  return (
    <Container disablePadding>
      <View style={[styles.stage, { paddingBottom: controlsHeight }]}>
        <GestureDetector gesture={gesture}>
          <View style={styles.sequencer}>
            <Canvas ref={canvasRef} style={styles.canvas}>
              <Grid />
              {patterns.map((pattern) => (
                <PatternShape key={pattern.instrumentName} pattern={pattern} />
              ))}
              <CenterDisc />
              {player.isPlaying && (
                <NotesHighlight
                  progressSV={player.progressSV}
                  playingStep={player.playingStep}
                  playingInstruments={player.playingInstruments}
                />
              )}
            </Canvas>
            <TempoReadout
              bpm={bpm}
              playingInstruments={player.playingInstruments}
            />
          </View>
        </GestureDetector>
      </View>
    </Container>
  );
};

export default DrumMachine;

const styles = StyleSheet.create({
  stage: {
    flex: 1,
    justifyContent: 'center',
  },
  sequencer: {
    width: size,
    height: size,
  },
  canvas: {
    width: size,
    height: size,
  },
  readout: {
    ...StyleSheet.absoluteFill,
    alignItems: 'center',
    justifyContent: 'center',
  },
  readoutContent: {
    alignItems: 'center',
  },
  bpm: {
    fontSize: 44,
    fontWeight: '300',
    letterSpacing: -1,
    fontVariant: ['tabular-nums'],
    color: colors.textPrimary,
  },
  bpmCaption: {
    fontSize: 11,
    fontWeight: '600',
    letterSpacing: 1.2,
    color: colors.textMuted,
  },
});
