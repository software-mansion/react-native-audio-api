import { AudioContext } from "react-native-audio-api";

export const oscillatorTestWithDetune = (audioContextRef: React.RefObject<AudioContext | null>) => {
  const oscillatorNode = audioContextRef.current!.createOscillator();
  oscillatorNode.connect(audioContextRef.current!.destination);
  oscillatorNode.start();
  oscillatorNode.detune.setValueAtTime(100, audioContextRef.current!.currentTime + 1);
  oscillatorNode.stop(audioContextRef.current!.currentTime + 2);
}

export const oscillatorTestWithGain = (audioContextRef: React.RefObject<AudioContext | null>) => {
  const oscillatorNode = audioContextRef.current!.createOscillator();
  const gain = audioContextRef.current!.createGain();
  oscillatorNode.connect(gain);
  gain.connect(audioContextRef.current!.destination);
  oscillatorNode.start();
  gain.gain.value = 0.5;
  gain.gain.linearRampToValueAtTime(0.0, audioContextRef.current!.currentTime + 1.5);
  gain.gain.linearRampToValueAtTime(1.5, audioContextRef.current!.currentTime + 3);
  oscillatorNode.stop(audioContextRef.current!.currentTime + 4.5);
}

export const oscillatorTestWithStereoPanner = (audioContextRef: React.RefObject<AudioContext | null>) => {
  const oscillatorNode = audioContextRef.current!.createOscillator();
  const pan = audioContextRef.current!.createStereoPanner();
  oscillatorNode.connect(pan);
  pan.connect(audioContextRef.current!.destination);
  oscillatorNode.start();
  pan.pan.linearRampToValueAtTime(1.0, audioContextRef.current!.currentTime + 1.5);
  pan.pan.linearRampToValueAtTime(-1.0, audioContextRef.current!.currentTime + 3);
  oscillatorNode.stop(audioContextRef.current!.currentTime + 4.5);
}
