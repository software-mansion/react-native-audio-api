import React, { FC, useMemo } from 'react';
import { Platform } from 'react-native';

import type { Log } from './log';
import {
  ButtonRow,
  Call,
  CallButton,
  ChoiceSelect,
  Field,
  Section,
  StatusRow,
  StatusText,
} from './ui';
import { DEVICE_CALLS, useDevices } from './useDevices';

function shortenDeviceLabel(label: string): string {
  if (label === 'System default') {
    return 'Sys def';
  }
  return label.split(' · ')[0] ?? label;
}

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

  const shortInputLabels = useMemo(
    () =>
      Object.fromEntries(
        inputOptions.map((option) => [option, shortenDeviceLabel(option)])
      ),
    [inputOptions]
  );

  return (
    <Section title="Devices" shortTitle="Dev">
      <Call text={DEVICE_CALLS.read} />
      <StatusRow>
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
      </StatusRow>
      <Field label="Input" shortLabel="In">
        <ButtonRow>
          <ChoiceSelect
            testID="device-input"
            value={selectedInputLabel}
            options={inputOptions}
            shortLabels={shortInputLabels}
            onChange={selectInput}
          />
          {Platform.OS === 'ios' && (
            <CallButton
              testID="device-output"
              title="Output route"
              shortTitle="Out route"
              call="none yet: AVAudioSession has no output setter; stands in for AVRoutePickerView"
              disabled
              onPress={() => {}}
            />
          )}
          <CallButton
            testID="device-refresh"
            title="Refresh"
            shortTitle="↻ Ref"
            call={DEVICE_CALLS.refresh}
            onPress={refresh}
          />
        </ButtonRow>
      </Field>
      <Call text={DEVICE_CALLS.setInput} />
    </Section>
  );
};

export default DevicePanel;
