import React, { FC } from 'react';
import { StyleSheet, View, ViewProps } from 'react-native';

import { colors, layout } from '../styles';

const Card: FC<ViewProps> = ({ style, ...props }) => (
  <View style={[styles.card, style]} {...props} />
);

export default Card;

const styles = StyleSheet.create({
  card: {
    backgroundColor: colors.surface,
    borderRadius: layout.cardRadius,
    padding: layout.screenPadding,
    gap: layout.spacing,
  },
});
