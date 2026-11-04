import { MenuView, type NativeActionEvent } from '@react-native-menu/menu';
import React from 'react';
import { StyleSheet, Text, View } from 'react-native';

import { colors } from '../styles';
import AppIcon from './AppIcon';

interface SelectProps<T extends string> {
  value: T;
  options: T[];
  onChange: (value: T) => void;
}

const height = 44;

/**
 * A capsule trigger that opens the platform pull-down menu (UIMenu on iOS,
 * which renders as liquid glass on iOS 26; PopupMenu on Android). The current
 * option is checkmarked on iOS.
 */
function Select<T extends string>(props: SelectProps<T>) {
  const { options, value, onChange } = props;

  const actions = options.map((option) => ({
    id: option,
    title: option,
    state: option === value ? ('on' as const) : ('off' as const),
  }));

  const onPressAction = ({ nativeEvent }: NativeActionEvent) => {
    const selected = options.find((option) => option === nativeEvent.event);
    if (selected !== undefined) {
      onChange(selected);
    }
  };

  return (
    <MenuView actions={actions} onPressAction={onPressAction}>
      <View
        style={styles.trigger}
        accessible
        accessibilityRole="button"
        accessibilityLabel={value}
        accessibilityHint="Opens a list of options"
      >
        <Text style={styles.value} numberOfLines={1}>
          {value}
        </Text>
        <AppIcon name="chevrons-up-down" size={16} color={colors.textMuted} />
      </View>
    </MenuView>
  );
}

export default Select;

const styles = StyleSheet.create({
  trigger: {
    height,
    borderRadius: height / 2,
    paddingHorizontal: 16,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 8,
    backgroundColor: colors.surface,
  },
  value: {
    flex: 1,
    fontSize: 15,
    color: colors.textPrimary,
  },
});
