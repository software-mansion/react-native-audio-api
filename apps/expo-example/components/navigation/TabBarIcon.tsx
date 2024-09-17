import { StyleSheet } from 'react-native';
import { type ComponentProps } from 'react';
import Ionicons from '@expo/vector-icons/Ionicons';
import { type IconProps } from '@expo/vector-icons/build/createIconSet';

export function TabBarIcon({
  style,
  ...rest
}: IconProps<ComponentProps<typeof Ionicons>['name']>) {
  return <Ionicons size={28} style={[styles.icon, style]} {...rest} />;
}

const styles = StyleSheet.create({
  icon: {
    marginBottom: -3,
  },
});
