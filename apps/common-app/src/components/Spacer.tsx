import { FC } from 'react';
import { View } from 'react-native';

interface SpacerProps {
  size: number;
}

const Vertical: FC<SpacerProps> = ({ size }) => {
  return <View style={{ height: size }} />;
};

const Horizontal: FC<SpacerProps> = ({ size }) => {
  return <View style={{ width: size }} />;
};

export default { Vertical, Horizontal };
