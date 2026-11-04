import React, { type FC } from 'react';
import { Switch as NativeSwitch } from 'react-native';

import { colors } from '../styles';

interface SwitchProps {
  value: boolean;
  onValueChange: (value: boolean) => void;
}

const trackColor = { false: colors.track, true: colors.accent };

const Switch: FC<SwitchProps> = ({ value, onValueChange }) => (
  <NativeSwitch
    value={value}
    onValueChange={onValueChange}
    trackColor={trackColor}
    ios_backgroundColor={colors.track}
    thumbColor={colors.white}
  />
);

export default Switch;
