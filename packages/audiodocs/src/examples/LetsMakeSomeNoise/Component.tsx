import React from 'react';
import { Button, View } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

export default function App() {
  const handlePlay = async () => {
    const audioContext = new AudioContext();

    const audioBuffer = await audioContext.decodeAudioDataSource(
      '/audio/music/example-music-01.mp3'
    );

    const playerNode = audioContext.createBufferSource();
    playerNode.buffer = audioBuffer;

    playerNode.connect(audioContext.destination);
    playerNode.start(audioContext.currentTime);
  };

  return (
    <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
      <Button onPress={handlePlay} title="Play sound!" />
    </View>
  );
}
