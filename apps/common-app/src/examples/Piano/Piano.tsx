import { FC, useEffect, useRef } from 'react';
import { AudioContext, AudioBuffer } from 'react-native-audio-api';
import * as FileSystem from 'expo-file-system';

import { Container } from '../../components';
import { KeyName, sources, keyMap } from './utils';
import PianoNote from './PianoNote';
import Keyboard from './Keyboard';

const Piano: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const notesRef = useRef<null | Record<KeyName, PianoNote>>(null);
  const bufferListRef = useRef<Partial<Record<KeyName, AudioBuffer>>>({});

  const onPressIn = (key: KeyName) => {
    notesRef.current?.[key].start(bufferListRef.current);
  };

  const onPressOut = (key: KeyName) => {
    notesRef.current?.[key].stop();
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    // eslint-disable-next-line @typescript-eslint/no-misused-promises
    Object.entries(sources).forEach(async ([key, url]) => {
      bufferListRef.current[key as KeyName] = await FileSystem.downloadAsync(
        url,
        FileSystem.documentDirectory + key.replace('#', 's') + '.mp3'
      ).then(({ uri }) => {
        return audioContextRef.current!.decodeAudioDataSource(uri);
      });
    });

    const newNotes: Partial<Record<KeyName, PianoNote>> = {};

    Object.values(keyMap).forEach((key) => {
      newNotes[key.name] = new PianoNote(audioContextRef.current!, key);
    });

    notesRef.current = newNotes as Record<KeyName, PianoNote>;

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <Container centered>
      <Keyboard keys={keyMap} onPressIn={onPressIn} onPressOut={onPressOut} />
    </Container>
  );
};

export default Piano;
