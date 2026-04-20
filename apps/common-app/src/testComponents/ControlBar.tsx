import React, { FC } from 'react';
import { StyleSheet, View } from 'react-native';

import { Button } from '../components';
import { layout } from '../styles';
import type { ControlAction } from './types';

interface ControlBarProps {
  actions: ControlAction[];
}

const ControlBar: FC<ControlBarProps> = ({ actions }) => (
  <View style={styles.controls}>
    {actions.map((action, index) => (
      <Button
        key={`${action.title}-${index}`}
        title={action.title}
        onPress={action.onPress}
        disabled={action.disabled}
        width={action.width}
      />
    ))}
  </View>
);

const styles = StyleSheet.create({
  controls: {
    flexDirection: 'row',
    gap: layout.spacing,
    paddingHorizontal: 12,
    paddingBottom: 12,
  },
});

export default ControlBar;
