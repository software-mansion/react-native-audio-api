import { useRef, useState } from 'react';
import { Container, Button } from '../../components';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
  BaseAudioContext,
  OfflineAudioContext,
} from 'react-native-audio-api';

AudioManager.setAudioSessionOptions({
  iosCategory: 'playAndRecord',
  iosOptions: ['defaultToSpeaker'],
});
AudioManager.requestRecordingPermissions();

function AudioRecorderBug() {
  const recorderRef = useRef(
    new AudioRecorder({ sampleRate: 44100, bufferLengthInSamples: 441 })
  );
  const [isStarted, setIsStarted] = useState(false);

  return (
    <Button
      title={isStarted ? 'Stop AudioRecorder' : 'Start AudioRecorder'}
      onPress={() => {
        if (isStarted) {
          recorderRef.current.stop();
        } else {
          recorderRef.current.start();
        }
        setIsStarted((b) => !b);
      }}
    />
  );
}

function audioParamTest(ac: BaseAudioContext) {
  const osc = ac.createOscillator();
  osc.connect(ac.destination);
  for (let i = 0; i < 500; i++) {
    osc.frequency.setValueAtTime(440 + i * 2, 0.01 * i);
  }
  osc.start();
  osc.stop(5);
}
function AudioParamBug() {
  return (
    <>
      <Button
        title="Regular audio context AudioParam test"
        onPress={() => {
          audioParamTest(new AudioContext());
        }}
      />
      <Button
        title="Offline audio context AudioParam test"
        onPress={async () => {
          const offlineAudioContext = new OfflineAudioContext({
            sampleRate: 44100,
            length: 44100 * 5,
            numberOfChannels: 2,
          });
          audioParamTest(offlineAudioContext);
          const buf = await offlineAudioContext.startRendering();

          const audioContext = new AudioContext();
          const node = audioContext.createBufferSource();
          node.buffer = buf;
          node.connect(audioContext.destination);
          node.start();
        }}
      />
    </>
  );
}

export default function Index() {
  return (
    <Container centered>
      <AudioRecorderBug />
      <AudioParamBug />
    </Container>
  );
}
