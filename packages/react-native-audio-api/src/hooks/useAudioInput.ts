import { useCallback, useEffect, useMemo, useState } from 'react';

import { AudioManager } from '../api';
import { OnRouteChangeEventType, RouteChangeReason } from '../events/types';
import {
  AudioDeviceInfo,
  AudioDeviceList,
  AudioDevicesInfo,
} from '../system/types';

const meaningfulReasons: RouteChangeReason[] = [
  'NewDeviceAvailable',
  'OldDeviceUnavailable',
  // e.g. system picks a different device as current one is not suitable for the new configuration
  'CategoryChange',
  'ConfigurationChange',
];

export default function useAudioInput() {
  const [availableInputs, setAvailableInputs] = useState<AudioDeviceList>([]);
  const [currentInput, setCurrentInput] = useState<string | null>(null);

  const onSelectInput = useCallback(async (device: AudioDeviceInfo) => {
    const success = await AudioManager.setInputDevice(device.uid);

    if (success) {
      setCurrentInput(device.uid);
    }

    const devicesInfo: AudioDevicesInfo = await AudioManager.getDevicesInfo();
    setAvailableInputs(devicesInfo.availableInputs);
  }, []);

  useEffect(() => {
    async function fetchAvailableInputs() {
      const audioDevices = await AudioManager.getDevicesInfo();
      const currentDeviceUid = audioDevices.currentInputs.length
        ? audioDevices.currentInputs[0].uid
        : null;

      setAvailableInputs(audioDevices.availableInputs);
      setCurrentInput(currentDeviceUid);
    }

    async function handleRouteChange(event: OnRouteChangeEventType) {
      if (!meaningfulReasons.includes(event.reason)) {
        return;
      }

      await fetchAvailableInputs();
    }

    const sub = AudioManager.addSystemEventListener(
      'routeChange',
      handleRouteChange
    );

    fetchAvailableInputs();
    return () => {
      sub?.remove();
    };
  }, []);

  return useMemo(
    () => ({
      availableInputs,
      currentInput: availableInputs.find((d) => d.uid === currentInput) || null,
      onSelectInput,
    }),
    [availableInputs, currentInput, onSelectInput]
  );
}
