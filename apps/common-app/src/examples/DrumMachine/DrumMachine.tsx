import { Canvas } from '@shopify/react-native-skia';
import React, { useState, useCallback } from 'react';
import { AudioContext } from 'react-native-audio-api';
import { GestureDetector } from 'react-native-gesture-handler';
import { LayoutChangeEvent, StyleSheet, View } from 'react-native';

import { colors } from '../../styles';
import { Select, Slider, Spacer, Container } from '../../components';
import { Kick, Clap, HiHat } from '../SharedUtils';

import { InstrumentName, Pattern, type XYWHRect } from './types';
import { size, initialBpm } from './constants';
import NotesHighlight from './NotesHighlight';
import PatternShape from './PatternShape';
import useGestures from './useGestures';
import PlayButton from './PlayButton';
import usePlayer from './usePlayer';
import presets from './presets';
import Grid from './Grid';

const defaultPreset = 'Empty';

function setupPlayer(audioCtx: AudioContext) {
  const kick = new Kick(audioCtx);
  // const clap = new Clap(audioCtx);
  const hiHat = new HiHat(audioCtx);

  const playNote = (name: InstrumentName, time: number) => {
    switch (name) {
      case 'kick':
        kick.play(time);
        break;
      // case 'clap':
      //   clap.play(time);
      //   break;
      case 'hi-hat':
        hiHat.play(time);
        break;
    }
  };

  return { playNote };
}

const DrumMachine: React.FC = () => {
  const [preset, setPreset] = useState<string>(defaultPreset);
  const [bpm, setBpm] = useState<number>(initialBpm);

  const [patterns, setPatterns] = useState<Pattern[]>([
    ...presets[defaultPreset].pattern,
  ]);

  const player = usePlayer({
    bpm,
    patterns,
    notesPerBeat: 2,
    setup: setupPlayer,
  });

  const [canvasRect, setCanvasRect] = useState<XYWHRect>({
    x: 0,
    y: 0,
    width: 0,
    height: 0,
  });

  const onPatternChange = useCallback(
    (patternIdx: number, stepIdx: number) => {
      if (preset !== 'Custom') {
        setPreset('Custom');
      }

      setPatterns((prevPatterns) => {
        const newPatterns = [...prevPatterns].map((pattern, idx) => {
          if (idx !== patternIdx) {
            return { ...pattern, steps: [...pattern.steps] };
          }

          const newPattern = { ...pattern, steps: [...pattern.steps] };
          newPattern.steps[stepIdx] = !newPattern.steps[stepIdx];

          return newPattern;
        });

        return newPatterns;
      });
    },
    [preset]
  );

  const onSetPreset = useCallback(
    (newPreset: string) => {
      setPreset(newPreset);

      if (newPreset !== 'Custom') {
        setBpm(presets[newPreset].bpm);
        setPatterns([...presets[newPreset].pattern]);
      }
    },
    [setPreset]
  );

  const onPlayPress = useCallback(() => {
    if (player.isPlaying) {
      player.stop();
    } else {
      player.play();
    }
  }, [player]);

  const onCanvasLayout = useCallback((event: LayoutChangeEvent) => {
    setCanvasRect({
      x: event.nativeEvent.layout.x,
      y: event.nativeEvent.layout.y,
      width: event.nativeEvent.layout.width,
      height: event.nativeEvent.layout.height,
    });
  }, []);

  const gesture = useGestures({ canvasRect, onPatternChange });

  return (
    <Container>
      <View>
        <Select
          value={preset}
          onChange={onSetPreset}
          options={Object.keys(presets)}
        />
        <Spacer.Vertical size={24} />
        <Slider
          label="BPM"
          step={1}
          min={24}
          max={320}
          value={bpm}
          onValueChange={setBpm}
        />
      </View>
      <Spacer.Vertical size={54} />
      <GestureDetector gesture={gesture}>
        <View style={styles.container}>
          <Canvas
            // @ts-ignore ???
            width={size}
            height={size}
            onLayout={onCanvasLayout}
          >
            <Grid />
            {patterns.map((pattern) => (
              <PatternShape key={pattern.instrumentName} pattern={pattern} />
            ))}
            {player.isPlaying && (
              <NotesHighlight
                progressSV={player.progressSV}
                playingInstruments={player.playingInstruments}
              />
            )}
          </Canvas>
          <PlayButton
            onPress={onPlayPress}
            canvasRect={canvasRect}
            isPlaying={player.isPlaying}
            playingInstruments={player.playingInstruments}
          />
        </View>
      </GestureDetector>
    </Container>
  );
};

export default DrumMachine;

const styles = StyleSheet.create({
  screen: {
    backgroundColor: colors.background,
    flex: 1,
    position: 'relative',
  },
  container: {
    flex: 1,
    position: 'relative',
    alignItems: 'center',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
  },
  label: {
    fontSize: 16,
    color: colors.white,
  },
});
