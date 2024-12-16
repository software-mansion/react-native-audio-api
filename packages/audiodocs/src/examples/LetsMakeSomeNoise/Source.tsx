import React from 'react';
import * as FileSystem from 'expo-file-system';
import { View, Button } from 'react-native';
import { AudioContext } from 'react-native-audio-api';

export default function App() {
  const handlePlay = async () => {
    const audioContext = new AudioContext();

    const audioBuffer = await FileSystem.downloadAsync(
      'https://docs.swmansion.com/audio-api/audio/music/example-music-01.mp3',
      `${FileSystem.documentDirectory}/example-music-01.mp3`
    ).then(({ uri }) => audioContext.decodeAudioDataSource(uri));

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
