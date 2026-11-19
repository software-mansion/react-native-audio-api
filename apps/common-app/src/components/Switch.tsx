import React, { type FC } from 'react';
import { Switch as NativeSwitch } from 'react-native';

import { colors } from '../styles';

interface SwitchProps {
  value: boolean;
  onValueChange: (value: boolean) => void;
  testID?: string;
}

const trackColor = { false: colors.track, true: colors.accent };

const Switch: FC<SwitchProps> = ({ value, onValueChange, testID }) => (
  <NativeSwitch
    testID={testID}
    value={value}
    onValueChange={onValueChange}
    trackColor={trackColor}
    ios_backgroundColor={colors.track}
    thumbColor={colors.white}
  />
);

export default Switch;
