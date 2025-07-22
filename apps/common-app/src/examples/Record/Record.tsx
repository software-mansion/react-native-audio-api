import React, { useRef, FC, useEffect, act } from 'react';
import {
  AudioContext,
  AudioManager,
  AudioRecorder,
  RecorderAdapterNode
} from 'react-native-audio-api';

import { Container, Button } from '../../components';

const Record: FC = () => {
  const recorderRef = useRef<AudioRecorder | null>(null);
  const aCtxRef = useRef<AudioContext | null>(null);
  const recorderAdapterRef = useRef<RecorderAdapterNode | null>(null);


  useEffect(() => {
    AudioManager.setAudioSessionOptions({
      iosCategory: 'playAndRecord',
      iosMode: 'spokenAudio',
      iosOptions: ['defaultToSpeaker', 'allowBluetoothA2DP'],
    });
    
    recorderRef.current = new AudioRecorder({
      sampleRate: 48000,
      bufferLengthInSamples: 48000,
    });
    aCtxRef.current = new AudioContext({ sampleRate: 48000 });
    recorderAdapterRef.current = aCtxRef.current.createRecorderAdapter();
    
    recorderRef.current.connect(recorderAdapterRef.current); // TODO this would be desired flow
    recorderAdapterRef.current.connect(aCtxRef.current.destination);
    console.log('Recorder adapter connected to audio context');
    
    if (aCtxRef.current.state === 'suspended') {
      console.log('Resuming audio context');
      aCtxRef.current.resume();
    }
  }, []);


  const start = () => {
    if (!aCtxRef.current || !recorderRef.current) {
      console.error('AudioContext or AudioRecorder is not initialized');
      return;
    }
    
    recorderRef.current.start();
    recorderRef.current.onAudioReady((audioBuffer) => {
      console.log('Audio ready:', audioBuffer);
    });
    console.log('Recording started'); 
    console.log('Audio context state:', aCtxRef.current.state);
    if (aCtxRef.current.state === 'suspended') {
      console.log('Resuming audio context');
      aCtxRef.current.resume();
    }
  }

  /// This stops only the recording, not the audio context
  const stopRecorder = () => {
    if (!recorderRef.current) {
      console.error('AudioRecorder is not initialized');
      return;
    }
    recorderRef.current.stop();
    console.log('Recording stopped');
  }

  return (
    <Container centered>
      <Button title="Start Recording" onPress={start} />
      <Button title="Stop Recording" onPress={stopRecorder} />
    </Container>
  );
};

export default Record;

// import React, { useRef, FC, useEffect } from 'react';
// import {
//   AudioBuffer,
//   AudioContext,
//   AudioManager,
//   AudioRecorder,
//   AudioBufferSourceNode,
// } from 'react-native-audio-api';

// import { Container, Button } from '../../components';

// const Record: FC = () => {
//   const recorderRef = useRef<AudioRecorder | null>(null);
//   const audioBuffersRef = useRef<AudioBuffer[]>([]);
//   const sourcesRef = useRef<AudioBufferSourceNode[]>([]);
//   const aCtxRef = useRef<AudioContext | null>(null);

//   useEffect(() => {
//     AudioManager.setAudioSessionOptions({
//       iosCategory: 'playAndRecord',
//       iosMode: 'spokenAudio',
//       iosOptions: ['allowBluetooth', 'defaultToSpeaker'],
//     });

//     recorderRef.current = new AudioRecorder({
//       sampleRate: 48000,
//       bufferLengthInSamples: 48000,
//     });
//   }, []);

//   const onReplay = () => {
//     const aCtx = new AudioContext({ sampleRate: 48000 });
//     aCtxRef.current = aCtx;

//     if (aCtx.state === 'suspended') {
//       aCtx.resume();
//     }

//     const tNow = aCtx.currentTime;
//     let nextStartAt = tNow + 1;
//     const buffers = audioBuffersRef.current;

//     console.log(tNow, nextStartAt, buffers.length);

//     for (let i = 0; i < buffers.length; i++) {
//       const source = aCtx.createBufferSource();
//       source.buffer = buffers[i];

//       source.connect(aCtx.destination);
//       // source.onended = () => {
//       //   console.log('Audio buffer source ended');
//       // };
//       sourcesRef.current.push(source);

//       source.start(nextStartAt);
//       nextStartAt += buffers[i].duration;
//     }

//     setTimeout(
//       () => {
//         console.log('clearing data');
//         audioBuffersRef.current = [];
//         sourcesRef.current = [];
//       },
//       (nextStartAt - tNow) * 1000
//     );
//   };

//   const onRecord = () => {
//     if (!recorderRef.current) {
//       return;
//     }

//     recorderRef.current.onAudioReady((event) => {
//       const { buffer, numFrames, when } = event;

//       console.log(
//         'Audio recorder buffer ready:',
//         buffer.duration,
//         numFrames,
//         when
//       );
//       audioBuffersRef.current.push(buffer);
//     });

//     recorderRef.current.start();

//     setTimeout(() => {
//       recorderRef.current?.stop();
//       console.log('Recording stopped');
//     }, 5000);
//   };

//   return (
//     <Container centered>
//       <Button title="Record" onPress={onRecord} />
//       <Button title="Replay" onPress={onReplay} />
//     </Container>
//   );
// };

// export default Record;