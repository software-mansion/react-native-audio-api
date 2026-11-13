import React from 'react';
import { StyleSheet, View } from 'react-native';

import { colors } from '../styles';
import AppIcon, { type AppIconName } from './AppIcon';

interface IconBadgeProps {
  icon: AppIconName;
  color: string;
  size?: number;
}

/** Light badge colours need a dark glyph to keep enough contrast. */
const darkGlyphBadges: readonly string[] = [colors.yellow];

/** A rounded-square colour tile holding a white glyph, as in iOS Settings rows. */
const IconBadge: React.FC<IconBadgeProps> = ({ icon, color, size = 30 }) => {
  const glyphColor = darkGlyphBadges.includes(color)
    ? colors.onAccent
    : colors.white;

  return (
    <View
      style={[
        styles.badge,
        {
          width: size,
          height: size,
          borderRadius: size * 0.27,
          backgroundColor: color,
        },
      ]}
    >
      <AppIcon
        name={icon}
        size={size * 0.6}
        color={glyphColor}
        accessible={false}
      />
    </View>
  );
};

export default IconBadge;

const styles = StyleSheet.create({
  badge: {
    alignItems: 'center',
    justifyContent: 'center',
  },
});
