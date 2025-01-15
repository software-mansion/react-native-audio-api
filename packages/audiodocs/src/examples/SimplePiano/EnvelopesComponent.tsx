import { View, Text, Pressable } from 'react-native';
import React, { FC, useEffect, useRef } from 'react';
import {
  GainNode,
  AudioBuffer,
  AudioContext,
  AudioBufferSourceNode,
} from 'react-native-audio-api';

type KeyName = 'A' | 'B' | 'C' | 'D' | 'E';

type PR<V> = Partial<Record<KeyName, V>>;

interface ButtonProps {
  keyName: KeyName;
  onPressIn: (key: KeyName) => void;
  onPressOut: (key: KeyName) => void;
}

interface PlayingNote {
  source: AudioBufferSourceNode;
  envelope: GainNode;
  startedAt: number;
}

const Keys = ['A', 'B', 'C', 'D', 'E'] as const;

const sourceList: PR<string> = {
  A: '/react-native-audio-api/audio/sounds/C4.mp3',
  C: '/react-native-audio-api/audio/sounds/Ds4.mp3',
  E: '/react-native-audio-api/audio/sounds/Fs4.mp3',
};

const Button = ({ onPressIn, onPressOut, keyName }: ButtonProps) => (
  <Pressable
    onPressIn={() => onPressIn(keyName)}
    onPressOut={() => onPressOut(keyName)}
    style={({ pressed }) => ({
      margin: 4,
      padding: 12,
      borderRadius: 2,
      backgroundColor: pressed ? '#d2e6ff' : '#abcdef',
    })}
  >
    <Text style={{ color: 'white' }}>{`${keyName}`}</Text>
  </Pressable>
);

const SimplePiano: FC = () => {
  const audioContextRef = useRef<AudioContext | null>(null);
  const playingNotesRef = useRef<PR<PlayingNote>>({});
  const bufferMapRef = useRef<PR<AudioBuffer>>({});

  const onKeyPressIn = (which: KeyName) => {
    const audioContext = audioContextRef.current;
    let buffer = bufferMapRef.current[which];
    const tNow = audioContext.currentTime;

    if (!audioContext || !buffer) {
      return;
    }

    const source = audioContext.createBufferSource();
    source.buffer = buffer;

    const envelope = audioContext.createGain();

    source.connect(envelope);
    envelope.connect(audioContext.destination);

    envelope.gain.setValueAtTime(0.001, tNow);
    envelope.gain.exponentialRampToValueAtTime(1, tNow + 0.1);

    source.start(tNow);

    playingNotesRef.current[which] = { source, envelope, startedAt: tNow };
  };

  const onKeyPressOut = (which: KeyName) => {
    const audioContext = audioContextRef.current!;
    const playingNote = playingNotesRef.current[which];

    if (!playingNote || !audioContext) {
      return;
    }

    const { source, envelope, startedAt } = playingNote;

    const tStop = Math.max(audioContext.currentTime, startedAt + 5);

    envelope.gain.exponentialRampToValueAtTime(0.0001, tStop + 0.08);
    envelope.gain.setValueAtTime(0, tStop + 0.09);
    source.stop(tStop + 0.1);

    playingNotesRef.current[which] = undefined;
  };

  useEffect(() => {
    if (!audioContextRef.current) {
      audioContextRef.current = new AudioContext();
    }

    Object.entries(sourceList).forEach(async ([key, url]) => {
      bufferMapRef.current[key as KeyName] =
        await audioContextRef.current!.decodeAudioDataSource(url);
    });

    return () => {
      audioContextRef.current?.close();
    };
  }, []);

  return (
    <View
      style={{
        flex: 1,
        justifyContent: 'center',
        alignItems: 'center',
        flexDirection: 'row',
      }}
    >
      {Keys.map((key) => (
        <Button
          onPressIn={onKeyPressIn}
          onPressOut={onKeyPressOut}
          keyName={key}
          key={key}
        />
      ))}
    </View>
  );
};

export default SimplePiano;
