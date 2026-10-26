import React, { FC, PropsWithChildren } from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { Switch } from '../../components';
import { colors, layout } from '../../styles';

export const Section: FC<PropsWithChildren<{ title: string }>> = ({
  title,
  children,
}) => (
  <View style={styles.section}>
    <Text style={styles.title}>{title}</Text>
    {children}
  </View>
);

export const Field: FC<PropsWithChildren<{ label: string }>> = ({
  label,
  children,
}) => (
  <View style={styles.field}>
    <Text style={styles.label}>{label}</Text>
    <View style={styles.fieldControl}>{children}</View>
  </View>
);

export const ButtonRow: FC<PropsWithChildren> = ({ children }) => (
  <View style={styles.buttonRow}>{children}</View>
);

export const StatusText: FC<{ testID: string; text: string }> = ({
  testID,
  text,
}) => (
  <Text testID={testID} style={styles.status}>
    {text}
  </Text>
);

interface LabeledSwitchProps {
  label: string;
  value: boolean;
  onValueChange: (value: boolean) => void;
  testID: string;
}

export const LabeledSwitch: FC<LabeledSwitchProps> = ({
  label,
  value,
  onValueChange,
  testID,
}) => (
  <View style={styles.field}>
    <Text style={styles.label}>{label}</Text>
    <Switch testID={testID} value={value} onValueChange={onValueChange} />
  </View>
);

const styles = StyleSheet.create({
  section: {
    gap: layout.spacing,
    paddingVertical: 2 * layout.spacing,
    borderBottomWidth: 1,
    borderBottomColor: colors.separator,
  },
  title: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '600',
  },
  field: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
  },
  label: {
    width: 80,
    color: colors.gray,
  },
  fieldControl: {
    flex: 1,
  },
  buttonRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: layout.spacing,
  },
  status: {
    color: colors.yellow,
  },
});
