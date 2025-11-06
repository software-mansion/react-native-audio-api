import { AudioContext } from "react-native-audio-api";

export const streamerTest = (audioContextRef: React.RefObject<AudioContext | null>) => {
  const streamer = audioContextRef.current!.createStreamer();
  streamer.initialize('https://stream.radioparadise.com/aac-320');
  streamer.connect(audioContextRef.current!.destination);
  streamer.start(audioContextRef.current!.currentTime);
  streamer.stop(audioContextRef.current!.currentTime + 5);
}
