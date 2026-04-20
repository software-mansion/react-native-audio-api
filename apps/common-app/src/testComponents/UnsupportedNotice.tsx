import React, { FC } from 'react';
import { StyleSheet, Text } from 'react-native';

import { Spacer } from '../components';
import { colors } from '../styles';

interface UnsupportedNoticeProps {
  title: string;
  message: string;
}

const UnsupportedNotice: FC<UnsupportedNoticeProps> = ({ title, message }) => (
  <>
    <Text style={styles.unsupportedTitle}>{title}</Text>
    <Spacer.Vertical size={12} />
    <Text style={styles.unsupportedText}>{message}</Text>
  </>
);

const styles = StyleSheet.create({
  unsupportedTitle: {
    color: colors.white,
    fontSize: 24,
    fontWeight: '700',
  },
  unsupportedText: {
    color: colors.gray,
    fontSize: 14,
    lineHeight: 16,
    maxWidth: 280,
    textAlign: 'center',
  },
});

export default UnsupportedNotice;
