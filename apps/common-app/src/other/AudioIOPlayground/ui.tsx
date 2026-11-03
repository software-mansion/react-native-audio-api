import React, { createContext, FC, PropsWithChildren, useContext } from 'react';
import { Platform, Pressable, StyleSheet, Text, View } from 'react-native';

import { Button, Select, Switch } from '../../components';
import { colors, layout } from '../../styles';
import MultiSelect from './MultiSelect';

export const monospace = Platform.select({
  ios: 'Menlo',
  default: 'monospace',
});

export const CompactContext = createContext(true);

export const useCompact = (): boolean => useContext(CompactContext);

/**
 * Fits "Output route", the longest title, so every caption starts in one
 * column.
 */
const BUTTON_WIDTH = 120;

export const Section: FC<
  PropsWithChildren<{ title: string; shortTitle?: string }>
> = ({ title, shortTitle, children }) => {
  const isCompact = useCompact();
  return (
    <View style={isCompact ? styles.compactSection : styles.section}>
      <Text style={isCompact ? styles.compactTitle : styles.title}>
        {isCompact ? (shortTitle ?? title) : title}
      </Text>
      {children}
    </View>
  );
};

export const Field: FC<
  PropsWithChildren<{ label: string; shortLabel?: string }>
> = ({ label, shortLabel, children }) => {
  const isCompact = useCompact();
  return (
    <View style={isCompact ? styles.compactField : styles.field}>
      <Text style={isCompact ? styles.compactLabel : styles.label}>
        {isCompact ? (shortLabel ?? label) : label}
      </Text>
      <View style={styles.fieldControl}>{children}</View>
    </View>
  );
};

/** The library call behind the control above it or beside it. */
export const Call: FC<{ text: string }> = ({ text }) => {
  const isCompact = useCompact();
  if (isCompact) {
    return null;
  }
  return <Text style={styles.call}>{text}</Text>;
};

export const ButtonRow: FC<PropsWithChildren> = ({ children }) => {
  const isCompact = useCompact();
  return (
    <View style={isCompact ? styles.compactButtonRow : styles.buttonColumn}>
      {children}
    </View>
  );
};

export const StatusRow: FC<PropsWithChildren> = ({ children }) => {
  const isCompact = useCompact();
  return (
    <View style={isCompact ? styles.compactStatusRow : styles.statusColumn}>
      {children}
    </View>
  );
};

interface CompactChipProps {
  testID?: string;
  label: string;
  selected?: boolean;
  action?: boolean;
  disabled?: boolean;
  onPress: () => void;
}

const CompactChip: FC<CompactChipProps> = ({
  testID,
  label,
  selected = false,
  action = false,
  disabled = false,
  onPress,
}) => (
  <Pressable
    testID={testID}
    disabled={disabled}
    onPress={onPress}
    style={({ pressed }) => [
      styles.chip,
      action && styles.actionChip,
      selected && styles.selectedChip,
      pressed && styles.pressedChip,
      disabled && styles.disabledChip,
    ]}
  >
    <Text
      style={[styles.chipText, (action || selected) && styles.activeChipText]}
    >
      {label}
    </Text>
  </Pressable>
);

interface CallButtonProps {
  testID: string;
  title: string;
  shortTitle?: string;
  call: string;
  onPress: () => void;
  disabled?: boolean;
}

/**
 * In Detailed mode: one action per row with its API call caption beside it. In
 * Compact mode: a compact action button using `shortTitle` so a full group of
 * actions fits on a single horizontal row.
 */
export const CallButton: FC<CallButtonProps> = ({
  testID,
  title,
  shortTitle,
  call,
  onPress,
  disabled,
}) => {
  const isCompact = useCompact();
  if (isCompact) {
    return (
      <CompactChip
        testID={testID}
        label={shortTitle ?? title}
        action
        disabled={disabled}
        onPress={onPress}
      />
    );
  }
  return (
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
};

export const StatusText: FC<{ testID: string; text: string }> = ({
  testID,
  text,
}) => {
  const isCompact = useCompact();
  return (
    <Text
      testID={testID}
      numberOfLines={isCompact ? 1 : undefined}
      style={isCompact ? styles.compactStatus : styles.status}
    >
      {text}
    </Text>
  );
};

interface LabeledSwitchProps {
  label: string;
  shortLabel?: string;
  value: boolean;
  onValueChange: (value: boolean) => void;
  testID: string;
}

export const LabeledSwitch: FC<LabeledSwitchProps> = ({
  label,
  shortLabel,
  value,
  onValueChange,
  testID,
}) => {
  const isCompact = useCompact();
  if (isCompact) {
    const tag = shortLabel ?? label;
    return (
      <CompactChip
        testID={testID}
        label={`${tag}: ${value ? 'ON' : 'off'}`}
        selected={value}
        onPress={() => onValueChange(!value)}
      />
    );
  }
  return (
    <View style={styles.field}>
      <Text style={styles.label}>{label}</Text>
      <Switch testID={testID} value={value} onValueChange={onValueChange} />
    </View>
  );
};

interface ChoiceSelectProps<T extends string> {
  testID: string;
  value: T;
  options: readonly T[];
  shortLabels?: Partial<Record<T, string>>;
  onChange: (value: T) => void;
}

export function ChoiceSelect<T extends string>({
  testID,
  value,
  options,
  shortLabels,
  onChange,
}: ChoiceSelectProps<T>) {
  const isCompact = useCompact();
  if (!isCompact) {
    return (
      <Select<T>
        testID={testID}
        value={value}
        options={[...options]}
        onChange={onChange}
      />
    );
  }
  return (
    <View style={styles.chipRow}>
      {options.map((option) => (
        <CompactChip
          key={option}
          testID={`${testID}-option-${option}`}
          label={shortLabels?.[option] ?? option}
          selected={option === value}
          onPress={() => onChange(option)}
        />
      ))}
    </View>
  );
}

interface MultiChoiceSelectProps<T extends string> {
  testID: string;
  values: readonly T[];
  options: readonly T[];
  shortLabels?: Partial<Record<T, string>>;
  onChange: (values: T[]) => void;
}

export function MultiChoiceSelect<T extends string>({
  testID,
  values,
  options,
  shortLabels,
  onChange,
}: MultiChoiceSelectProps<T>) {
  const isCompact = useCompact();
  if (!isCompact) {
    return (
      <MultiSelect<T>
        testID={testID}
        values={[...values]}
        options={[...options]}
        onChange={onChange}
      />
    );
  }
  const toggle = (option: T) => {
    const next = values.includes(option)
      ? values.filter((item) => item !== option)
      : [...values, option];
    onChange(next);
  };
  return (
    <View style={styles.chipRow}>
      {options.map((option) => (
        <CompactChip
          key={option}
          testID={`${testID}-option-${option}`}
          label={shortLabels?.[option] ?? option}
          selected={values.includes(option)}
          onPress={() => toggle(option)}
        />
      ))}
    </View>
  );
}

const styles = StyleSheet.create({
  section: {
    gap: layout.spacing,
    paddingVertical: 2 * layout.spacing,
    borderBottomWidth: 1,
    borderBottomColor: colors.separator,
  },
  compactSection: {
    gap: 3,
    paddingVertical: 4,
    borderBottomWidth: StyleSheet.hairlineWidth,
    borderBottomColor: colors.separator,
  },
  title: {
    color: colors.white,
    fontSize: 18,
    fontWeight: '600',
  },
  compactTitle: {
    color: colors.gray,
    fontSize: 10,
    fontWeight: '700',
    textTransform: 'uppercase',
  },
  field: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
  },
  compactField: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: 4,
  },
  label: {
    width: 80,
    color: colors.gray,
  },
  compactLabel: {
    width: 38,
    color: colors.gray,
    fontSize: 10,
  },
  fieldControl: {
    flex: 1,
  },
  buttonColumn: {
    gap: layout.spacing,
  },
  compactButtonRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 4,
    alignItems: 'center',
  },
  statusColumn: {
    gap: layout.spacing,
  },
  compactStatusRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    columnGap: 8,
    rowGap: 2,
    alignItems: 'center',
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
  compactStatus: {
    color: colors.yellow,
    fontSize: 11,
    flexShrink: 1,
  },
  chipRow: {
    flexDirection: 'row',
    flexWrap: 'wrap',
    gap: 3,
    alignItems: 'center',
  },
  chip: {
    paddingVertical: 3,
    paddingHorizontal: 6,
    borderRadius: layout.radius,
    borderWidth: 1,
    borderColor: colors.border,
    backgroundColor: colors.backgroundDark,
  },
  actionChip: {
    paddingVertical: 4,
    paddingHorizontal: 8,
    borderColor: colors.main,
    backgroundColor: colors.main,
  },
  selectedChip: {
    borderColor: colors.main,
    backgroundColor: `${colors.main}55`,
  },
  pressedChip: {
    opacity: 0.65,
  },
  disabledChip: {
    opacity: 0.4,
  },
  chipText: {
    color: colors.gray,
    fontSize: 11,
  },
  activeChipText: {
    color: colors.white,
    fontWeight: '600',
  },
});
