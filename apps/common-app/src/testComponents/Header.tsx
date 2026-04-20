import React, { FC } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';

interface HeaderProps {
  title: string;
  subtitle?: string;
}

const Header: FC<HeaderProps> = ({ title, subtitle }) => (
  <View style={styles.header}>
    <Text style={styles.title}>{title}</Text>
    {subtitle ? <Text style={styles.subtitle}>{subtitle}</Text> : null}
  </View>
);

const styles = StyleSheet.create({
  header: {
    gap: 6,
    padding: 12,
  },
  title: {
    color: colors.white,
    fontSize: 24,
    fontWeight: '700',
  },
  subtitle: {
    color: colors.gray,
    fontSize: 14,
    lineHeight: 16,
  },
});

export default Header;
