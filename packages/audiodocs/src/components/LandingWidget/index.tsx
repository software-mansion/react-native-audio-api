import { useColorMode } from '@docusaurus/theme-common';
import React, { useState, useEffect, useCallback } from "react";

// import { Spacer } from '@site/src/components/Layout';
// import Speaker from '@site/static/icons/speaker.svg';
import AudioManager, { BufferMetadata } from "@site/src/audio/AudioManager";
// import SpeakerCrossed from '@site/static/icons/speaker-crossed.svg';
import { presetEffects } from "@site/src/audio/effects";

import { AudioType } from "./types";
import Spectrogram from "./Spectrogram";
import styles from "./styles.module.css";
import clsx from 'clsx';

const sounds: Record<AudioType, string | null> = {
  music: '/react-native-audio-api/audio/music/example-music-05.wav',
  speech: '/react-native-audio-api/audio/voice/voice-sample-landing.mp3',
  bgm: '/react-native-audio-api/audio/bgm/bgm-01.wav',
  efx: '/react-native-audio-api/audio/efx/efx-01.wav',
  guitar: null,
};

const showAmpToolbarThreshold = 30;

const LandingWidget: React.FC = () => {
  const { colorMode } = useColorMode();

  const [isMuted, setIsMuted] = useState(true);
  const [selectCount, setSelectCount] = useState(0);
  const [isLoading, setIsLoading] = useState(true);
  const [activeSources, setActiveSources] = useState<Record<AudioType, boolean>>({
    music: false,
    speech: false,
    bgm: false,
    efx: false,
    guitar: false,
  });
  const [selectedAudio, setSelectedAudio] = useState<AudioType>('music');
  const [selectedInput, setSelectedInput] = useState<string | null>(null);
  const [availableInputs, setAvailableInputs] = useState<MediaDeviceInfo[]>([]);
  const [availableSounds, setAvailableSounds] = useState<Record<AudioType, BufferMetadata | null>>({
    music: null,
    speech: null,
    bgm: null,
    efx: null,
    guitar: null,
  });

  const onSelectGuitar = useCallback(async () => {
    setSelectedAudio('guitar');
    setActiveSources(({
      music: false,
      speech: false,
      bgm: false,
      efx: false,
      guitar: true,
    }));

    try {
      if (selectedAudio === 'guitar') {
        AudioManager.disconnectMicrophone();
      }


      const device = availableInputs.find(input => (input.label || input.deviceId) === selectedInput);

      const constraints = {
        audio: {
          deviceId: device ? { exact: device.deviceId } : undefined,
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
          sampleRate: 44100,
          channelCount: 1
        }
      };

      const stream = await navigator.mediaDevices.getUserMedia(constraints);

      if (AudioManager.aCtx.state === 'suspended') {
        await AudioManager.aCtx.resume();
      }

      AudioManager.connectMicrophone(stream);

      setTimeout(() => {
        const effectsMap = presetEffects.distorted();
        AudioManager.connectMicrophone(stream, effectsMap);
      }, 1000);

      setIsMuted(false);
    } catch (error) {
      console.error('Failed to access microphone:', error);
    }
  }, [selectedAudio, selectedInput, availableInputs]);

  // const onToggleSpeaker = useCallback(() => {
  //   // If we're in the hidden toolbar state (selectCount >= 10), only allow muting
  //   if (selectCount >= 10) {
  //     if (!isMuted) {
  //       setIsMuted(true);

  //       if (selectedAudio === 'guitar') {
  //         AudioManager.disconnectMicrophone();
  //       } else {
  //         const currentSound = availableSounds[selectedAudio];
  //         if (currentSound && currentSound.id) {
  //           AudioManager.stopSound(currentSound.id);
  //         }
  //       }
  //     } else {
  //       setIsMuted(false);

  //       if (selectedAudio === 'guitar') {
  //         onSelectGuitar();
  //         return;
  //       }

  //       const currentSound = availableSounds[selectedAudio];

  //       if (!currentSound) {
  //         return;
  //       }

  //       AudioManager.playSound(currentSound.id, 0, () => {
  //         setIsMuted(true);
  //       });
  //       return;
  //     }

  //     return;
  //   }

  //   if (isMuted) {
  //     setIsMuted(false);

  //     if (selectedAudio === 'guitar') {
  //       onSelectGuitar();
  //       return;
  //     }

  //     const currentSound = availableSounds[selectedAudio];

  //     if (!currentSound) {
  //       return;
  //     }

  //     AudioManager.playSound(currentSound.id, 0, () => {
  //       setIsMuted(true);
  //     });

  //     return;
  //   }

  //   setIsMuted(true);

  //   if (selectedAudio === 'guitar') {
  //     AudioManager.disconnectMicrophone();
  //     return;
  //   }

  //   const currentSound = availableSounds[selectedAudio];
  //   if (currentSound && currentSound.id) {
  //     AudioManager.stopSound(currentSound.id);
  //   }
  // }, [isMuted, selectedAudio, availableSounds, onSelectGuitar, selectCount]);

  const onSelectAudio = useCallback(async (audioType: AudioType) => {
    setSelectCount(prev => prev + 1);
    setSelectedAudio(audioType);

    setActiveSources((prev) => ({
      ...prev,
      [audioType]: true,
    }));

    setIsMuted(false);

    if (audioType === 'guitar') {
      await onSelectGuitar();
      return;
    }

    const sound = availableSounds[audioType];

    if (!sound || !sound.id) {
      console.warn(`No sound available for type: ${audioType}`);
      return;
    }

    if (AudioManager.isActive(sound.id)) {
      AudioManager.stopSound(sound.id);
      setActiveSources((prev) => ({
        ...prev,
        [audioType]: false,
      }));
      return;
    } else {
      AudioManager.playSound(sound.id, audioType, 0, () => {
        setActiveSources((prev) => ({
          ...prev,
          [audioType]: false,
        }));
      }, audioType === 'bgm');
    }
  }, [availableSounds, onSelectGuitar]);

  const onSelectClean = useCallback(async () => {
    try {
      if (selectedAudio === 'guitar') {
        AudioManager.disconnectMicrophone();
      }

      const device = availableInputs.find(input => (input.label || input.deviceId) === selectedInput);

      const constraints = {
        audio: {
          deviceId: device ? { exact: device.deviceId } : undefined,
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
          sampleRate: 44100,
          channelCount: 1
        }
      };

      const stream = await navigator.mediaDevices.getUserMedia(constraints);

      if (AudioManager.aCtx.state === 'suspended') {
        await AudioManager.aCtx.resume();
      }

      // Connect microphone with clean effects (just a gain boost)
      const cleanEffects = presetEffects.test(); // Uses gain 0.3
      AudioManager.connectMicrophone(stream, cleanEffects);

      setSelectedAudio('guitar');
      setActiveSources(({
        music: false,
        speech: false,
        bgm: false,
        efx: false,
        guitar: true,
      }));
      setIsMuted(false);
    } catch (error) {
      console.error('Failed to access microphone for clean sound:', error);
    }
  }, [selectedAudio, selectedInput, availableInputs]);

  const onSelectDistorted = useCallback(async () => {
    try {
      // Stop any current microphone audio
      if (selectedAudio === 'guitar') {
        AudioManager.disconnectMicrophone();
      }

      // Get microphone access
      const devices = await navigator.mediaDevices.enumerateDevices();
      const audioInputs = devices.find(device => device.kind === 'audioinput');

      const constraints = {
        audio: {
          deviceId: audioInputs ? { exact: audioInputs.deviceId } : undefined,
          echoCancellation: false,
          noiseSuppression: false,
          autoGainControl: false,
          sampleRate: 44100,
          channelCount: 1
        }
      };

      const stream = await navigator.mediaDevices.getUserMedia(constraints);

      if (AudioManager.aCtx.state === 'suspended') {
        await AudioManager.aCtx.resume();
      }

      // Connect microphone with distorted effects
      const distortedEffects = presetEffects.distorted();
      AudioManager.connectMicrophone(stream, distortedEffects);

      setSelectedAudio('guitar');
      setIsMuted(false);
    } catch (error) {
      console.error('Failed to access microphone for distorted sound:', error);
    }
  }, [selectedAudio]);

  useEffect(() => {
    async function fetchAudioInputs() {
      try {
        const devices = await navigator.mediaDevices.enumerateDevices();
        const audioInputs = devices.filter(device => device.kind === 'audioinput');

        setAvailableInputs(audioInputs);

        if (audioInputs.length > 0) {
          setSelectedInput(audioInputs[0].label || audioInputs[0].deviceId);
        }
      } catch (error) {
        console.error('Failed to fetch audio inputs:', error);
      }
    }

    fetchAudioInputs();
  }, []);

  useEffect(() => {
    function loadSounds() {
      const promises = Object.entries(sounds).map(([key, path]) => {
        if (!path) {
          return Promise.resolve(null);
        }

        return AudioManager.loadSound(path);
      });

      Promise.all(promises)
        .then((loadedSounds) => {
          const soundsMap: Record<AudioType, BufferMetadata | null> = {
            music: null,
            speech: null,
            bgm: null,
            efx: null,
            guitar: null,
          };

          loadedSounds.forEach((sound, index) => {
            const audioType = Object.keys(sounds)[index] as AudioType;
            soundsMap[audioType] = sound;
          });

          setAvailableSounds(soundsMap);
          setIsLoading(false);
        });
    }

    loadSounds();
  }, []);

  return (
    <div className={styles.container}>
      {selectCount >= showAmpToolbarThreshold && (
        <div className={styles.hiddenToolbar}>
          <div className={styles.toolbarButtonsGroup}>
            <button className={styles.widgetToolbarButton} onClick={onSelectClean}>Clean</button>
            <button className={styles.widgetToolbarButton} onClick={onSelectDistorted}>Distorted</button>

          </div>
        </div>
      )}
      <div className={styles.toolbar}>
        {/* <div className={styles.toolbarButtonsSingle}>
          <button className={styles.muteButton} onClick={onToggleSpeaker}>
            {isMuted ? <SpeakerCrossed color={colorMode === 'dark' ? 'var(--swm-red-dark-100)' : 'var(--swm-red-light-100)'} /> : <Speaker color={colorMode === 'dark' ? 'var(--swm-blue-dark-100)' : 'var(--swm-blue-light-100)'} />}
          </button>
        </div>
        <Spacer.H size="12px" /> */}
        {!isLoading && (
          <div className={styles.toolbarButtonsGroup}>
            <button className={!activeSources.music ? styles.widgetToolbarButton : styles.widgetToolbarButtonActive} onClick={() => onSelectAudio('music')}>Music</button>
            <button className={!activeSources.speech ? styles.widgetToolbarButton : styles.widgetToolbarButtonActive} onClick={() => onSelectAudio('speech')}>Speech</button>
            <button className={!activeSources.bgm ? styles.widgetToolbarButton : styles.widgetToolbarButtonActive} onClick={() => onSelectAudio('bgm')}>BGM</button>
            <button className={!activeSources.efx ? styles.widgetToolbarButton : styles.widgetToolbarButtonActive} onClick={() => onSelectAudio('efx')}>efx</button>
          </div>
        )}
      </div>
      <Spectrogram selectedAudio={selectedAudio} />
    </div>
  )
}

export default LandingWidget;
