import React from 'react';
import { useState, FC } from 'react';
import { Container, Button } from '../../components';

const AudioFile: FC = () => {
  const [isPlaying, setIsPlaying] = useState(false);

  return (
    <Container centered>
      <Button
        title={isPlaying ? 'Stop' : 'Play'}
        onPress={() => setIsPlaying(!isPlaying)}
      />
    </Container>
  );
};

export default AudioFile;
