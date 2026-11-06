import React from 'react';
import { Pressable, StyleSheet, Text } from 'react-native';

import { colors } from '../styles';
import AppIcon, { type AppIconName } from './AppIcon';
import IconBadge from './IconBadge';

interface ListRowProps {
  title: string;
  icon: AppIconName;
  iconColor: string;
  onPress: () => void;
}

/** A navigation row for `ListSection`: badge, title and a trailing chevron. */
const ListRow: React.FC<ListRowProps> = ({
  title,
  icon,
  iconColor,
  onPress,
}) => (
  <Pressable
    onPress={onPress}
    accessibilityRole="button"
    accessibilityLabel={title}
    style={({ pressed }) => [styles.row, pressed && styles.pressed]}
  >
    <IconBadge icon={icon} color={iconColor} />
    <Text style={styles.title} numberOfLines={1}>
      {title}
    </Text>
    <AppIcon
      name="chevron-right"
      size={18}
      color={colors.textMuted}
      accessible={false}
    />
  </Pressable>
);

export default ListRow;

const styles = StyleSheet.create({
  row: {
    minHeight: 52,
    paddingLeft: 12,
    paddingRight: 14,
    flexDirection: 'row',
    alignItems: 'center',
    gap: 12,
  },
  pressed: {
    backgroundColor: colors.surfacePressed,
  },
  title: {
    flex: 1,
    fontSize: 17,
    color: colors.textPrimary,
  },
});
