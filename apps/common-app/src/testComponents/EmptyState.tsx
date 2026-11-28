import React, { FC } from 'react';
import { Text } from 'react-native';

import { Card } from '../components';
import { typography } from '../styles';

const EmptyState: FC<{ message: string }> = ({ message }) => (
  <Card>
    <Text style={typography.hint}>{message}</Text>
  </Card>
);

export default EmptyState;
