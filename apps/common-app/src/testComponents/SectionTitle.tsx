import React, { FC } from 'react';
import { StyleSheet, Text } from 'react-native';

import { colors } from '../styles';

interface SectionTitleProps {
  title: string;
}

const SectionTitle: FC<SectionTitleProps> = ({ title }) => (
  <Text style={styles.sectionTitle}>{title}</Text>
);

const styles = StyleSheet.create({
  sectionTitle: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '700',
  },
});

export default SectionTitle;
