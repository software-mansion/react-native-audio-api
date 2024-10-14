import { FC, useEffect, useRef } from 'react';
import { AudioContext } from 'react-native-audio-api';

import { Container } from '../../components';
import { KeyName, keyMap } from './utils';
import PianoNote from './PianoNote';
import Keyboard from './Keyboard';

const Piano: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const notesRef = useRef<null | Record<KeyName, PianoNote>>(null);

  const onPressIn = (key: KeyName) => {
    notesRef.current?.[key].start();
  };

  const onPressOut = (key: KeyName) => {
    notesRef.current?.[key].stop();
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    const newNotes: Partial<Record<KeyName, PianoNote>> = {};

    Object.values(keyMap).forEach((key) => {
      newNotes[key.name] = new PianoNote(audioContextRef.current!, key);
    });

    notesRef.current = newNotes as Record<KeyName, PianoNote>;
  }, []);

  return (
    <Container centered>
      <Keyboard keys={keyMap} onPressIn={onPressIn} onPressOut={onPressOut} />
    </Container>
  );
};

export default Piano;
