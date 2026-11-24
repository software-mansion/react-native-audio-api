import React, { FC } from 'react';
import { Text } from 'react-native';

import { typography } from '../styles';

const SectionTitle: FC<{ title: string }> = ({ title }) => (
  <Text style={typography.sectionTitle}>{title}</Text>
);

export default SectionTitle;
