import { AudioContext } from "react-native-audio-api";

export const workletTest = (audioContextRef: React.RefObject<AudioContext | null>) => {
    const processingWorklet = (
      inputAudioData: Array<Float32Array>,
      outputAudioData: Array<Float32Array>,
      framesToProcess: number,
      _currentTime: number
    ) => {
      'worklet';
      const gain = 0.1;
      for (let channel = 0; channel < inputAudioData.length; channel++) {
        const inputChannelData = inputAudioData[channel];
        const outputChannelData = outputAudioData[channel];
        for (let i = 0; i < framesToProcess; i++) {
          outputChannelData[i] = inputChannelData[i] * gain;
        }
      }
    };

    const workletNode = audioContextRef.current!.createWorkletProcessingNode(
      processingWorklet,
      'AudioRuntime'
    );

    const oscillatorNode = audioContextRef.current!.createOscillator();
    oscillatorNode.connect(workletNode);
    workletNode.connect(audioContextRef.current!.destination);
    oscillatorNode.start();
    oscillatorNode.stop(audioContextRef.current!.currentTime + 4);
}
