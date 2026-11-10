import { useCallback, useEffect, useState } from 'react';
import { Platform } from 'react-native';
import {
  AudioDeviceInfo,
  AudioDevicesInfo,
  AudioManager,
} from 'react-native-audio-api';

import { describeCall, describeError, Log, logAction } from './log';

const SYSTEM_DEFAULT = 'System default';

const CALLS = {
  read: 'AudioManager.getDevicesInfo()',
  setInput: 'AudioManager.setInputDevice',
};

export const DEVICE_CALLS = {
  read: `${CALLS.read}, again on every 'routeChange' event`,
  setInput: `${CALLS.setInput}(id), or ${CALLS.setInput}('default') for ${SYSTEM_DEFAULT}`,
  refresh: CALLS.read,
};

const EMPTY_DEVICES: AudioDevicesInfo = {
  availableInputs: [],
  availableOutputs: [],
  currentInputs: [],
  currentOutputs: [],
};

/** Same label the Record demo builds for its input picker. */
function deviceLabel(device: AudioDeviceInfo): string {
  return `${device.name} · ${device.category} #${device.id}`;
}

function formatDeviceList(devices: AudioDeviceInfo[]): string {
  return devices.length === 0 ? 'none' : devices.map(deviceLabel).join(', ');
}

/**
 * On Android an empty current-device list means the library cannot tell, not
 * that there is no device: `currentOutputs` is never filled, and
 * `currentInputs` is empty until a take opens a stream or an input is selected.
 * Printing `unavailable` keeps that gap from reading as `none`.
 */
function formatCurrentDevices(devices: AudioDeviceInfo[]): string {
  if (Platform.OS === 'android' && devices.length === 0) {
    return 'unavailable';
  }
  return formatDeviceList(devices);
}

/**
 * Reads the route and selects an input through `setInputDevice`. `'default'`
 * clears the selection.
 */
export function useDevices(log: Log) {
  const [devices, setDevices] = useState<AudioDevicesInfo>(EMPTY_DEVICES);
  const [selectedInputId, setSelectedInputId] = useState<string | null>(null);

  const readDevices = useCallback(async () => {
    const info = await AudioManager.getDevicesInfo();
    setDevices(info);
    setSelectedInputId(info.currentInputs[0]?.id ?? null);
  }, []);

  useEffect(() => {
    logAction(log, CALLS.read, readDevices);

    const subscription = AudioManager.addSystemEventListener(
      'routeChange',
      () => {
        readDevices().catch((error) =>
          log(
            `${CALLS.read} after routeChange: failed: ${describeError(error)}`
          )
        );
      }
    );

    return () => subscription?.remove();
  }, [log, readDevices]);

  const selectInput = useCallback(
    (label: string) => {
      if (label === SYSTEM_DEFAULT) {
        logAction(log, describeCall(CALLS.setInput, 'default'), async () => {
          await AudioManager.setInputDevice('default');
          await readDevices();
        });
        return;
      }

      const device = devices.availableInputs.find(
        (input) => deviceLabel(input) === label
      );
      if (!device) {
        return;
      }

      logAction(log, describeCall(CALLS.setInput, device.id), async () => {
        await AudioManager.setInputDevice(device.id);
        setSelectedInputId(device.id);
        setDevices(await AudioManager.getDevicesInfo());
      });
    },
    [devices.availableInputs, log, readDevices]
  );

  const selectedDevice = devices.availableInputs.find(
    (input) => input.id === selectedInputId
  );

  return {
    currentInputs: formatCurrentDevices(devices.currentInputs),
    currentOutputs: formatCurrentDevices(devices.currentOutputs),
    availableOutputs: formatDeviceList(devices.availableOutputs),
    inputOptions: [SYSTEM_DEFAULT, ...devices.availableInputs.map(deviceLabel)],
    selectedInputLabel: selectedDevice
      ? deviceLabel(selectedDevice)
      : SYSTEM_DEFAULT,
    selectInput,
    refresh: () => logAction(log, CALLS.read, readDevices),
  };
}
