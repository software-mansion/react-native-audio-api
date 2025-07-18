import React, { useState, useEffect, useCallback } from "react";

import { Spacer } from '@site/src/components/Layout';
import Speaker from '@site/static/icons/speaker.svg';
import AudioManager, { BufferMetadata } from "@site/src/audio/AudioManager";
import SpeakerCrossed from '@site/static/icons/speaker-crossed.svg';
import { presetEffects } from "@site/src/audio/effects";

import { AudioType } from "./types";
import Spectrogram from "./Spectrogram";
import styles from "./styles.module.css";

const sounds: Record<AudioType, string | null> = {
  music: '/react-native-audio-api/audio/music/example-music-02.mp3',
  speech: '/react-native-audio-api/audio/music/example-music-02.mp3',
  audio: '/react-native-audio-api/audio/music/example-music-02.mp3',
  guitar: null,
};

const LandingWidget: React.FC = () => {
  const [isMuted, setIsMuted] = useState(true);
  const [selectCount, setSelectCount] = useState(0);
  const [selectedAudio, setSelectedAudio] = useState<AudioType>('music');
  const [availableSounds, setAvailableSounds] = useState<Record<AudioType, BufferMetadata | null>>({
    music: null,
    speech: null,
    audio: null,
    guitar: null,
  });

  const onSelectGuitar = useCallback(async () => {
    setSelectedAudio('guitar');

    try {
      if (!isMuted) {
        const currentSound = availableSounds[selectedAudio];
        if (currentSound && currentSound.id) {
          await AudioManager.stopSound(currentSound.id);
        }
      }

      const devices = await navigator.mediaDevices.enumerateDevices();
      const audioInputs = devices.filter(device => device.kind === 'audioinput');

      const usbDevice = audioInputs.find(device =>
        device.label && (
          device.label.toLowerCase().includes('usb') ||
          device.label.toLowerCase().includes('interface') ||
          device.label.toLowerCase().includes('audio') ||
          device.label.toLowerCase().includes('guitar') ||
          device.label.toLowerCase().includes('amp')
        )
      );

      const constraints = {
        audio: {
          deviceId: usbDevice ? { exact: usbDevice.deviceId } : undefined,
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
  }, [selectedAudio, availableSounds, isMuted]);

  const onToggleSpeaker = useCallback(() => {
    // If we're in the hidden toolbar state (selectCount >= 10), only allow muting
    if (selectCount >= 10) {
      if (!isMuted) {
        setIsMuted(true);

        if (selectedAudio === 'guitar') {
          AudioManager.disconnectMicrophone();
        } else {
          const currentSound = availableSounds[selectedAudio];
          if (currentSound && currentSound.id) {
            AudioManager.stopSound(currentSound.id);
          }
        }
      } else {
        setIsMuted(false);

        if (selectedAudio === 'guitar') {
          onSelectGuitar();
          return;
        }

        const currentSound = availableSounds[selectedAudio];

        if (!currentSound) {
          return;
        }

        AudioManager.playSound(currentSound.id, 0, () => {
          setIsMuted(true);
        });
        return;
      }

      return;
    }

    if (isMuted) {
      setIsMuted(false);

      if (selectedAudio === 'guitar') {
        onSelectGuitar();
        return;
      }

      const currentSound = availableSounds[selectedAudio];

      if (!currentSound) {
        return;
      }

      AudioManager.playSound(currentSound.id, 0, () => {
        setIsMuted(true);
      });

      return;
    }

    setIsMuted(true);

    if (selectedAudio === 'guitar') {
      AudioManager.disconnectMicrophone();
      return;
    }

    const currentSound = availableSounds[selectedAudio];
    if (currentSound && currentSound.id) {
      AudioManager.stopSound(currentSound.id);
    }
  }, [isMuted, selectedAudio, availableSounds, onSelectGuitar, selectCount]);

  const onSelectAudio = useCallback(async (audioType: AudioType) => {
    setSelectCount(prev => prev + 1);
    setSelectedAudio(audioType);

    if (isMuted) {
      return;
    }

    if (selectedAudio === 'guitar') {
      AudioManager.disconnectMicrophone();
    } else {
      const currentSound = availableSounds[selectedAudio];
      if (currentSound && currentSound.id) {
        await AudioManager.stopSound(currentSound.id);
      }
    }

    if (audioType === 'guitar') {
      await onSelectGuitar();
    } else {
      const newSound = availableSounds[audioType];
      if (newSound) {
        AudioManager.playSound(newSound.id, 0, () => {
          setIsMuted(true);
        });
      }
    }
  }, [availableSounds, selectedAudio, isMuted, onSelectGuitar, selectCount]);

  const onSelectClean = useCallback(async () => {

    try {
      // Stop any current audio
      if (selectedAudio === 'guitar') {
        AudioManager.disconnectMicrophone();
      } else {
        const currentSound = availableSounds[selectedAudio];
        if (currentSound && currentSound.id && AudioManager.isPlaying && !isMuted) {
          await AudioManager.stopSound(currentSound.id);
        }
      }

      // Get microphone access
      const devices = await navigator.mediaDevices.enumerateDevices();
      const audioInputs = devices.filter(device => device.kind === 'audioinput');

      const usbDevice = audioInputs.find(device =>
        device.label && (
          device.label.toLowerCase().includes('usb') ||
          device.label.toLowerCase().includes('interface') ||
          device.label.toLowerCase().includes('audio') ||
          device.label.toLowerCase().includes('guitar') ||
          device.label.toLowerCase().includes('amp')
        )
      );

      const constraints = {
        audio: {
          deviceId: usbDevice ? { exact: usbDevice.deviceId } : undefined,
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
      setIsMuted(false);
    } catch (error) {
      console.error('Failed to access microphone for clean sound:', error);
    }
  }, [selectedAudio, availableSounds, isMuted]);

  const onSelectDistorted = useCallback(async () => {
    setSelectedAudio('guitar');

    try {
      // Stop any current audio
      if (selectedAudio === 'guitar') {
        AudioManager.disconnectMicrophone();
      } else {
        const currentSound = availableSounds[selectedAudio];
        if (currentSound && currentSound.id && AudioManager.isPlaying && !isMuted) {
          await AudioManager.stopSound(currentSound.id);
        }
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

      setIsMuted(false);
    } catch (error) {
      console.error('Failed to access microphone for distorted sound:', error);
    }
  }, [selectedAudio, availableSounds, isMuted]);

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
            audio: null,
            guitar: null,
          };

          loadedSounds.forEach((sound, index) => {
            const audioType = Object.keys(sounds)[index] as AudioType;
            soundsMap[audioType] = sound;
          });

          setAvailableSounds(soundsMap);
        });
    }

    loadSounds();
  }, []);

  return (
    <div className={styles.container}>
      {selectCount >= 10 && (
        <div className={styles.hiddenToolbar}>
          <div className={styles.toolbarButtonsGroup}>
            <button className={styles.toolbarButton} onClick={onSelectClean}>Clean</button>
            <button className={styles.toolbarButton} onClick={onSelectDistorted}>Distorted</button>

          </div>
        </div>
      )}
      <div className={styles.toolbar}>
        <div className={styles.toolbarButtonsSingle}>
          <button className={styles.muteButton} onClick={onToggleSpeaker}>
            {isMuted ? <SpeakerCrossed /> : <Speaker />}
          </button>
        </div>
        <Spacer.H size="12px" />
        <div className={styles.toolbarButtonsGroup}>
          <button className={styles.toolbarButton} onClick={() => onSelectAudio('music')}>Music</button>
          <button className={styles.toolbarButton} onClick={() => onSelectAudio('speech')}>Speech</button>
        </div>
      </div>
      <Spectrogram selectedAudio={selectedAudio} />
    </div>
  )
}

export default LandingWidget;
