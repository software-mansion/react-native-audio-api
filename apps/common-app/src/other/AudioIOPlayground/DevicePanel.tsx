import React, { FC } from 'react';
import { Platform } from 'react-native';

import { Button, Select } from '../../components';
import type { Log } from './log';
import { ButtonRow, Field, Section, StatusText } from './ui';
import { useDevices } from './useDevices';

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
      <StatusText
        testID="device-current-inputs"
        text={`current inputs: ${currentInputs}`}
      />
      <StatusText
        testID="device-current-outputs"
        text={`current outputs: ${currentOutputs}`}
      />
      <Field label="Input">
        <Select
          testID="device-input"
          value={selectedInputLabel}
          options={inputOptions}
          onChange={selectInput}
        />
      </Field>
      {Platform.OS === 'ios' ? (
        // AVAudioSession has no output setter. This stands in for AVRoutePickerView.
        <Button
          testID="device-output"
          title="Output route"
          width={140}
          disabled
          onPress={() => {}}
        />
      ) : (
        <StatusText
          testID="device-available-outputs"
          text={`available outputs: ${availableOutputs}`}
        />
      )}
      <ButtonRow>
        <Button testID="device-refresh" title="Refresh" onPress={refresh} />
      </ButtonRow>
    </Section>
  );
};

export default DevicePanel;
