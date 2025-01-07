import { AudioContext } from 'react-native-audio-api';
import React, { useState, useCallback, FC } from 'react';

import { Container, Slider, Spacer, Button } from '../../components';
import MetronomeSound from '../../utils/soundEngines/MetronomeSound';
import { InstrumentName, Pattern } from '../../types';
import { patterns, defaultPattern } from './patterns';
import usePlayer from '../../utils/usePlayer';

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

  const onBeatsPerBarChange = useCallback((newBeatsPerBar: number) => {
    setBeatsPerBar(newBeatsPerBar);
    setPattern([...patterns[newBeatsPerBar]]);
  }, []);

  const onPlayPress = useCallback(() => {
    if (player.isPlaying) {
      player.stop();
    } else {
      player.play();
    }
  }, [player]);

  return (
    <Container centered>
      <Button
        title={player.isPlaying ? 'Stop' : 'Play'}
        onPress={onPlayPress}
      />
      <Spacer.Vertical size={20} />
      <Slider
        label="BPM"
        step={1}
        min={24}
        max={320}
        value={bpm}
        onValueChange={setBpm}
      />
      <Spacer.Vertical size={20} />
      <Slider
        min={1}
        max={8}
        step={1}
        value={beatsPerBar}
        label="Beats"
        onValueChange={onBeatsPerBarChange}
        minLabelWidth={50}
      />
    </Container>
  );
};

export default Metronome;
