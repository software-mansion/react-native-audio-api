import { AudioContext } from 'react-native-audio-api';
import React, { useState, FC } from 'react';
import { StyleSheet } from 'react-native';

import { Container, Slider, Button } from '../../components';
import MetronomeSound from '../../utils/soundEngines/MetronomeSound';
import usePlayer, { InstrumentName, Pattern } from '../../utils/usePlayer';
import { patterns, defaultPattern } from './patterns';

const DOWN_BEAT_FREQUENCY = 1000;
const REGULAR_BEAT_FREQUENCY = 500;

const INITIAL_BPM = 120;
const INITIAL_BEATS_PER_BAR = 4;

function setupPlayer(audioCtx: AudioContext) {
  const downbeat = new MetronomeSound(audioCtx, DOWN_BEAT_FREQUENCY);
  const regularbeat = new MetronomeSound(audioCtx, REGULAR_BEAT_FREQUENCY);

  const playNote = (name: InstrumentName, time: number) => {
    switch (name) {
      case 'downbeat':
        downbeat.play(time);
        break;
      case 'regularbeat':
        regularbeat.play(time);
        break;
    }
  };

  return { playNote };
}

const Metronome: FC = () => {
  const [bpm, setBpm] = useState(INITIAL_BPM);
  const [beatsPerBar, setBeatsPerBar] = useState(INITIAL_BEATS_PER_BAR);
  const [pattern, setPattern] = useState<Pattern[]>([
    ...patterns[defaultPattern],
  ]);

  const player = usePlayer({
    bpm,
    patterns: pattern,
    notesPerBeat: 1,
    numBeats: beatsPerBar,
    setup: setupPlayer,
  });

  const onBeatsPerBarChange = (newBeatsPerBar: number) => {
    setBeatsPerBar(newBeatsPerBar);
    setPattern([...patterns[newBeatsPerBar]]);
  };

  return (
    <Container centered style={styles.content}>
      <Button
        title={player.isPlaying ? 'Stop' : 'Play'}
        onPress={player.isPlaying ? player.stop : player.play}
      />
      <Slider
        label="BPM"
        step={1}
        min={24}
        max={320}
        value={bpm}
        onValueChange={setBpm}
      />
      <Slider
        min={1}
        max={8}
        step={1}
        value={beatsPerBar}
        label="Beats"
        onValueChange={onBeatsPerBarChange}
      />
    </Container>
  );
};

export default Metronome;

const styles = StyleSheet.create({
  content: {
    gap: 20,
  },
});
