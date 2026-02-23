import React, { useRef } from 'react';
import { Button, View } from 'react-native';
import { AudioBufferSourceNode, AudioContext } from 'react-native-audio-api';

export default function App() {
  const absnRef = useRef<AudioBufferSourceNode | null>(null);

  const handlePlay = async () => {
    if (absnRef.current) {
      absnRef.current.stop();
    }

    const audioContext = new AudioContext();

    const audioBuffer = await fetch('/react-native-audio-api/audio/music/example-music-01.mp3')
      .then((response) => response.arrayBuffer())
      .then((arrayBuffer) => audioContext.decodeAudioData(arrayBuffer));

    const playerNode = await audioContext.createBufferSource();
    playerNode.buffer = audioBuffer;

    playerNode.connect(audioContext.destination);
    playerNode.start(audioContext.currentTime);
    playerNode.stop(audioContext.currentTime + 10);
    absnRef.current = playerNode;
  };

  return (
    <View style={{ flex: 1, justifyContent: 'center', alignItems: 'center' }}>
      <Button title="Play Sound!" onPress={handlePlay} color="#ff6259" />
    </View>
  );
}
