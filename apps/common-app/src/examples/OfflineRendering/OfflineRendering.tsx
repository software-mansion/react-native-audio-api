import React, { useCallback, useState, FC } from 'react';
import {
  AudioBuffer,
  AudioContext,
  OfflineAudioContext,
} from 'react-native-audio-api';

import { Container, Button } from '../../components';

const URL = 'https://download.samplelib.com/mp3/sample-12s.mp3';

const AudioFile: FC = () => {
  const [rendering, setRendering] = useState(false);
  const [renderedBuffer, setRenderedBuffer] = useState<AudioBuffer | null>(
    null
  );

  const handleStartRendering = useCallback(() => {
    if (rendering) return;
    setRendering(true);
    (async () => {
      const duration = 5;
      const sampleRate = 48_000;
      const length = duration * sampleRate;
      const offlineAudioContext = new OfflineAudioContext(
        2,
        length,
        sampleRate
      );

      // Suspend at 1 second
      offlineAudioContext
        .suspend(1)
        .then(() => {
          setTimeout(() => {
            offlineAudioContext.resume().catch((e) => console.error(e));
          }, 3000);
        })
        .catch((e) => console.error(e));

      const buffer = await fetch(URL)
        .then((response) => response.arrayBuffer())
        .then((arrayBuffer) =>
          offlineAudioContext.decodeAudioData(arrayBuffer)
        );
      const audioBufferSourceNode = offlineAudioContext.createBufferSource();
      audioBufferSourceNode.buffer = buffer;

      audioBufferSourceNode.start();
      audioBufferSourceNode.connect(offlineAudioContext.destination);
      const result = await offlineAudioContext.startRendering();
      setRenderedBuffer(result);
    })();
  }, [rendering]);

  const handleStartPlay = useCallback(() => {
    if (!renderedBuffer) return;
    const audioContext = new AudioContext();
    const audioBufferSourceNode = audioContext.createBufferSource();
    audioBufferSourceNode.buffer = renderedBuffer;
    audioBufferSourceNode.connect(audioContext.destination);
    audioBufferSourceNode.start();
  }, [renderedBuffer]);

  let buttonTitle = '';
  if (renderedBuffer) buttonTitle = 'Play';
  else if (rendering) buttonTitle = 'Rendering..';
  else buttonTitle = 'Start Rendering';

  const disabled = rendering && !renderedBuffer;
  const handler = !rendering ? handleStartRendering : handleStartPlay;

  return (
    <Container centered>
      <Button title={buttonTitle} onPress={handler} disabled={disabled} />
    </Container>
  );
};

export default AudioFile;
