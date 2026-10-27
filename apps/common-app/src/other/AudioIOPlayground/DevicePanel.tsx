import React, { FC } from 'react';
import { Platform } from 'react-native';

import { Select } from '../../components';
import type { Log } from './log';
import { Call, CallButton, Field, Section, StatusText } from './ui';
import { DEVICE_CALLS, useDevices } from './useDevices';

const DevicePanel: FC<{ log: Log }> = ({ log }) => {
  const {
    currentInputs,
    currentOutputs,
    availableOutputs,
    inputOptions,
    selectedInputLabel,
    selectInput,
    refresh,
  } = useDevices(log);

  return (
    <Section title="Devices">
      <Call text={DEVICE_CALLS.read} />
      <StatusText
        testID="device-current-inputs"
        text={`currentInputs: ${currentInputs}`}
      />
      <StatusText
        testID="device-current-outputs"
        text={`currentOutputs: ${currentOutputs}`}
      />
      {Platform.OS === 'android' && (
        <StatusText
          testID="device-available-outputs"
          text={`availableOutputs: ${availableOutputs}`}
        />
      )}
      <Field label="Input">
        <Select
          testID="device-input"
          value={selectedInputLabel}
          options={inputOptions}
          onChange={selectInput}
        />
      </Field>
      <Call text={DEVICE_CALLS.setInput} />
      {Platform.OS === 'ios' && (
        <CallButton
          testID="device-output"
          title="Output route"
          call="none yet: AVAudioSession has no output setter; stands in for AVRoutePickerView"
          disabled
          onPress={() => {}}
        />
      )}
      <CallButton
        testID="device-refresh"
        title="Refresh"
        call={DEVICE_CALLS.refresh}
        onPress={refresh}
      />
    </Section>
  );
};

export default DevicePanel;
