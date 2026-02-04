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

/**
 * A hook that provides basic information and selection capabilities for audio
 * input devices on the system. (iOS only currently). The hook will
 * automatically listen for configuration changes and updates its state. If you
 * need more granular control, consider using the AudioManager API directly.
 *
 * @returns An object containing audio input information and selection
 *   capabilities
 */
export default function useAudioInput() {
  const [availableInputs, setAvailableInputs] = useState<AudioDeviceList>([]);
  const [currentInput, setCurrentInput] = useState<string | null>(null);

  const onSelectInput = useCallback(async (device: AudioDeviceInfo) => {
    const success = await AudioManager.setInputDevice(device.id);

    if (success) {
      setCurrentInput(device.id);
    }

    const devicesInfo: AudioDevicesInfo = await AudioManager.getDevicesInfo();
    setAvailableInputs(devicesInfo.availableInputs);
  }, []);

  useEffect(() => {
    async function fetchAvailableInputs() {
      const audioDevices = await AudioManager.getDevicesInfo();
      const currentDeviceId = audioDevices.currentInputs.length
        ? audioDevices.currentInputs[0].id
        : null;

      setAvailableInputs(audioDevices.availableInputs);
      setCurrentInput(currentDeviceId);
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
      /**
       * The list of available audio input devices under current device
       * configuration.
       */
      availableInputs,
      /**
       * The currently selected audio input device, or null if none is yet
       * decided by the system.
       */
      currentInput: availableInputs.find((d) => d.id === currentInput) || null,
      /**
       * Selects the given device as the current input. Returns true if
       * successful, throws otherwise.
       */
      onSelectInput,
    }),
    [availableInputs, currentInput, onSelectInput]
  );
}
