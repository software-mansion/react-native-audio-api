import React, { useEffect, useRef, useState } from 'react';
import { ActivityIndicator, View } from 'react-native';
import { AudioContext, AudioFileSourceNode } from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { layout } from '../../styles';

/** Small public WAV for demo (works with miniaudio file decoder). */
const DEMO_AUDIO_URL =
  'https://filesamples.com/samples/audio/aac/sample4.aac';
  // '/Users/michal/Library/Developer/CoreSimulator/Devices/BD13B970-9C15-4FB4-91C8-38CBFFFDD3B8/data/Containers/Data/Application/40127BF3-30D7-44BD-9AFC-34DB21106188/Documents/audio.aac';

const AudioStream: React.FC = () => {
  const [playing, setPlaying] = useState(false);
  const [loading, setLoading] = useState(false);
  const [buffer, setBuffer] = useState<ArrayBuffer | string | null>(null);

  const ctxRef = useRef<AudioContext | null>(null);
  const sourceRef = useRef<AudioFileSourceNode | null>(null);

  useEffect(() => {
    ctxRef.current = new AudioContext();
    setLoading(true);

    if (DEMO_AUDIO_URL.startsWith('https')) {
      fetch(DEMO_AUDIO_URL)
          .then((r) => r.arrayBuffer())
          .then(setBuffer)
          .finally(() => setLoading(false));
    } else {
      setBuffer(DEMO_AUDIO_URL);
      setLoading(false);
    }
    return () => {
      sourceRef.current?.stop(0);
      ctxRef.current?.close();
    };
  }, []);

  const toggle = async () => {
    const ctx = ctxRef.current;
    if (!ctx || !buffer) return;

    if (playing) {
      sourceRef.current?.stop(ctx.currentTime);
      sourceRef.current = null;
      setPlaying(false);
      return;
    }

    await ctx.resume();

    const source = ctx.createAudioFileSource(buffer);
    source.connect(ctx.destination);
    source.start(ctx.currentTime);

    sourceRef.current = source;
    setPlaying(true);
  };

  return (
    <Container disablePadding>
      <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
        {loading && <ActivityIndicator color="#fff" />}
        <View style={{ marginTop: layout.spacing * 2 }}>
          <Button
            onPress={toggle}
            title={playing ? 'Stop' : 'Play'}
            disabled={!buffer || loading}
          />
        </View>
      </View>
    </Container>
  );
};

export default AudioStream;
