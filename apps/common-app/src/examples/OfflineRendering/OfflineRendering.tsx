import React, { FC, useEffect, useRef, useState } from 'react';
import { AudioBuffer, OfflineAudioContext } from 'react-native-audio-api';

import { Button, Container } from '../../components';
import { useAudioContext } from '../../utils/audio';

const URL = 'https://download.samplelib.com/mp3/sample-12s.mp3';

const RENDER_DURATION_S = 5;
const RENDER_SAMPLE_RATE = 48_000;
/** The render pauses here to exercise `suspend` / `resume`. */
const SUSPEND_AT_S = 1;
const SUSPEND_FOR_MS = 3000;

const OfflineRendering: FC = () => {
  const [isRendering, setIsRendering] = useState(false);
  const [renderedBuffer, setRenderedBuffer] = useState<AudioBuffer | null>(
    null
  );
  const audioContext = useAudioContext();
  const resumeTimeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null);

  useEffect(
    () => () => {
      if (resumeTimeoutRef.current) {
        clearTimeout(resumeTimeoutRef.current);
      }
    },
    []
  );

  const render = async () => {
    setIsRendering(true);

    try {
      const offlineAudioContext = new OfflineAudioContext(
        2,
        RENDER_DURATION_S * RENDER_SAMPLE_RATE,
        RENDER_SAMPLE_RATE
      );

      offlineAudioContext
        .suspend(SUSPEND_AT_S)
        .then(() => {
          resumeTimeoutRef.current = setTimeout(() => {
            offlineAudioContext.resume().catch((e) => console.error(e));
          }, SUSPEND_FOR_MS);
        })
        .catch((e) => console.error(e));

      const arrayBuffer = await fetch(URL).then((response) =>
        response.arrayBuffer()
      );
      const source = offlineAudioContext.createBufferSource();
      source.buffer = await offlineAudioContext.decodeAudioData(arrayBuffer);
      source.connect(offlineAudioContext.destination);
      source.start();

      setRenderedBuffer(await offlineAudioContext.startRendering());
    } catch (error) {
      console.error('Offline rendering failed:', error);
    } finally {
      setIsRendering(false);
    }
  };

  const play = () => {
    if (!renderedBuffer) {
      return;
    }
    const source = audioContext.createBufferSource();
    source.buffer = renderedBuffer;
    source.connect(audioContext.destination);
    source.onended = () => source.disconnect();
    source.start();
  };

  return (
    <Container centered>
      {renderedBuffer ? (
        <Button title="Play" onPress={play} />
      ) : (
        <Button
          title={isRendering ? 'Rendering…' : 'Start Rendering'}
          onPress={render}
          disabled={isRendering}
        />
      )}
    </Container>
  );
};

export default OfflineRendering;
