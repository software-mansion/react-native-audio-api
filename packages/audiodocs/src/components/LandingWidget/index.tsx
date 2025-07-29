import clsx from 'clsx';
import { useColorMode } from '@docusaurus/theme-common';
import React, { useState, useEffect, useCallback, useMemo, act } from "react";

import { Spacer } from '@site/src/components/Layout';
import AudioManager from "@site/src/audio/AudioManager";
import { presetEffects } from "@site/src/audio/effects";

import WidgetToolbar from './WidgetToolbar';
import styles from "./styles.module.css";
import Spectrogram from "./Spectrogram";
import { AudioMode, AudioSource, AudioSourceMetadata, SourceRecord } from './types';

const showAmpToolbarThreshold = 30;

const soundSources: SourceRecord<string> = {
  music: '/react-native-audio-api/audio/music/example-music-05.wav',
  speech: '/react-native-audio-api/audio/voice/voice-sample-landing.mp3',
  bgm: '/react-native-audio-api/audio/bgm/bgm-01.wav',
  efx: '/react-native-audio-api/audio/efx/efx-01.wav',
};

const initialSounds: SourceRecord<AudioSourceMetadata | null> = {
  music: null,
  speech: null,
  bgm: null,
  efx: null,
};

const labels: Record<AudioSource, string> = {
  music: 'Music',
  speech: 'Speech',
  bgm: 'BGM',
  efx: 'EFX'
};

const LandingWidget: React.FC = () => {
  const [isLoading, setIsLoading] = useState(true);
  const [mode, setMode] = useState<AudioMode>('player');
  const [sounds, setSounds] = useState<SourceRecord<AudioSourceMetadata | null>>(initialSounds);
  const [activeSounds, setActiveSounds] = useState<Array<{ id: string, type: AudioSource, startedAt: number }>>([]);

  const onPlaySound = useCallback(async (soundId: string, source: AudioSource) => {
    if (mode === 'amplifier') {
      // TODO: Handle running microphone mode
      setMode('player');
    }

    const sound = sounds[source];

    if (!sound || !sound.id) {
      return;
    }

    const alreadyActiveSound = activeSounds.find(s => s.type === source);

    if (alreadyActiveSound) {
      await AudioManager.stopSound(alreadyActiveSound.id);
      setActiveSounds(prev => prev.filter(s => s.type !== source));
      return;
    }

    const activeSound = AudioManager.playSound(sound.id, {
      loop: source === 'bgm',
      onEnded: () => {
        setActiveSounds(prev => prev.filter(s => s.type !== source));
      }
    });

    if (!activeSound) {
      return;
    }

    setActiveSounds(prev => [...prev, { id: activeSound.id, type: source, startedAt: activeSound.startedAt }]);
  }, [sounds, activeSounds]);

  useEffect(() => {
    function loadSounds() {
      const promises = Object
        .entries(soundSources)
        .map(async ([key, path]) => ({
          key,
          ...(await AudioManager.loadSound(path)),
        }));

      Promise.all(promises)
        .then((results) => {
          const newSounds = { ...initialSounds };

          results.forEach(({ key, ...metadata }) => {
            newSounds[key] = metadata;
          });

          setSounds(newSounds);
          setIsLoading(false);
        });
    }

    loadSounds();
  }, []);

  const buttons = useMemo(() => {
    const buttonList = Object.entries(sounds).map(([key, sound]) => {
      if (!sound || !sound.id) {
        return null;
      }

      return {
        id: sound.id,
        name: labels[key as AudioSource],
        type: key as AudioSource,
        isActive: activeSounds.some(s => s.type === key),
        onPlaySound,
      };
    });

    return buttonList.filter(Boolean);
  }, [sounds, onPlaySound, activeSounds]);

  useEffect(() => {
    return () => {
      AudioManager.clear();
    };
  }, [])

  return (
    <div className={styles.container}>
      <div className={styles.toolbar}>
        <Spacer.H size="12px" />
        <WidgetToolbar isLoading={isLoading} onPlaySound={onPlaySound} buttons={buttons} />
      </div>
      <Spectrogram />
    </div>
  );
}

export default LandingWidget;

// const [selectCount, setSelectCount] = useState(0);
// const [selectedInput, setSelectedInput] = useState<string | null>(null);
// const [availableInputs, setAvailableInputs] = useState<MediaDeviceInfo[]>([]);

// const onSelectGuitar = useCallback(async () => {
//   setSelectedAudio('guitar');
//   setActiveSources(({
//     music: false,
//     speech: false,
//     bgm: false,
//     efx: false,
//     guitar: true,
//   }));

//   try {
//     if (selectedAudio === 'guitar') {
//       AudioManager.disconnectMicrophone();
//     }


//     const device = availableInputs.find(input => (input.label || input.deviceId) === selectedInput);

//     const constraints = {
//       audio: {
//         deviceId: device ? { exact: device.deviceId } : undefined,
//         echoCancellation: false,
//         noiseSuppression: false,
//         autoGainControl: false,
//         sampleRate: 44100,
//         channelCount: 1
//       }
//     };

//     const stream = await navigator.mediaDevices.getUserMedia(constraints);

//     if (AudioManager.aCtx.state === 'suspended') {
//       await AudioManager.aCtx.resume();
//     }

//     AudioManager.connectMicrophone(stream);

//     setTimeout(() => {
//       const effectsMap = presetEffects.distorted();
//       AudioManager.connectMicrophone(stream, effectsMap);
//     }, 1000);

//     setIsMuted(false);
//   } catch (error) {
//     console.error('Failed to access microphone:', error);
//   }
// }, [selectedAudio, selectedInput, availableInputs]);

// const onSelectClean = useCallback(async () => {
//   try {
//     if (selectedAudio === 'guitar') {
//       AudioManager.disconnectMicrophone();
//     }

//     const device = availableInputs.find(input => (input.label || input.deviceId) === selectedInput);

//     const constraints = {
//       audio: {
//         deviceId: device ? { exact: device.deviceId } : undefined,
//         echoCancellation: false,
//         noiseSuppression: false,
//         autoGainControl: false,
//         sampleRate: 44100,
//         channelCount: 1
//       }
//     };

//     const stream = await navigator.mediaDevices.getUserMedia(constraints);

//     if (AudioManager.aCtx.state === 'suspended') {
//       await AudioManager.aCtx.resume();
//     }

//     // Connect microphone with clean effects (just a gain boost)
//     const cleanEffects = presetEffects.test(); // Uses gain 0.3
//     AudioManager.connectMicrophone(stream, cleanEffects);

//     setSelectedAudio('guitar');
//     setActiveSources(({
//       music: false,
//       speech: false,
//       bgm: false,
//       efx: false,
//       guitar: true,
//     }));
//     setIsMuted(false);
//   } catch (error) {
//     console.error('Failed to access microphone for clean sound:', error);
//   }
// }, [selectedAudio, selectedInput, availableInputs]);

// const onSelectDistorted = useCallback(async () => {
//   try {
//     // Stop any current microphone audio
//     if (selectedAudio === 'guitar') {
//       AudioManager.disconnectMicrophone();
//     }

//     // Get microphone access
//     const devices = await navigator.mediaDevices.enumerateDevices();
//     const audioInputs = devices.find(device => device.kind === 'audioinput');

//     const constraints = {
//       audio: {
//         deviceId: audioInputs ? { exact: audioInputs.deviceId } : undefined,
//         echoCancellation: false,
//         noiseSuppression: false,
//         autoGainControl: false,
//         sampleRate: 44100,
//         channelCount: 1
//       }
//     };

//     const stream = await navigator.mediaDevices.getUserMedia(constraints);

//     if (AudioManager.aCtx.state === 'suspended') {
//       await AudioManager.aCtx.resume();
//     }

//     // Connect microphone with distorted effects
//     const distortedEffects = presetEffects.distorted();
//     AudioManager.connectMicrophone(stream, distortedEffects);

//     setSelectedAudio('guitar');
//     setIsMuted(false);
//   } catch (error) {
//     console.error('Failed to access microphone for distorted sound:', error);
//   }
// }, [selectedAudio]);

// useEffect(() => {
//   async function fetchAudioInputs() {
//     try {
//       const devices = await navigator.mediaDevices.enumerateDevices();
//       const audioInputs = devices.filter(device => device.kind === 'audioinput');

//       setAvailableInputs(audioInputs);

//       if (audioInputs.length > 0) {
//         setSelectedInput(audioInputs[0].label || audioInputs[0].deviceId);
//       }
//     } catch (error) {
//       console.error('Failed to fetch audio inputs:', error);
//     }
//   }

//   fetchAudioInputs();
// }, []);
