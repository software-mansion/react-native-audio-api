import React, { FC, PropsWithChildren } from 'react';
import { Platform, StyleSheet, Text, View } from 'react-native';

import { Button, Switch } from '../../components';
import { colors, layout } from '../../styles';

export const monospace = Platform.select({
  ios: 'Menlo',
  default: 'monospace',
});

/**
 * Fits "Output route", the longest title, so every caption starts in one
 * column.
 */
const BUTTON_WIDTH = 120;

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

/** The library call behind the control above it or beside it. */
export const Call: FC<{ text: string }> = ({ text }) => (
  <Text style={styles.call}>{text}</Text>
);

interface CallButtonProps {
  testID: string;
  title: string;
  call: string;
  onPress: () => void;
  disabled?: boolean;
}

/** One action per row, so a button never moves when a caption wraps. */
export const CallButton: FC<CallButtonProps> = ({
  testID,
  title,
  call,
  onPress,
  disabled,
}) => (
  <View style={styles.callButton}>
    <Button
      testID={testID}
      title={title}
      onPress={onPress}
      disabled={disabled}
      width={BUTTON_WIDTH}
    />
    <View style={styles.fieldControl}>
      <Call text={call} />
    </View>
  </View>
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
  callButton: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
  },
  call: {
    color: colors.gray,
    fontSize: 11,
    fontFamily: monospace,
  },
  status: {
    color: colors.yellow,
  },
});
