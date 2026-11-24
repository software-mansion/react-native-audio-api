import { useNavigation } from '@react-navigation/native';
import type { FC } from 'react';
import React from 'react';
import { Pressable, StyleSheet, Text } from 'react-native';

import { IconBadge } from '../components';
import { demos } from '../demos';
import { colors, layout } from '../styles';
import TabScreen from './TabScreen';

const DemoList: FC = () => {
  const navigation = useNavigation();

  return (
    <TabScreen title="Demo Apps">
      {demos.map((demo) => (
        <Pressable
          key={demo.key}
          onPress={() => navigation.navigate(demo.key)}
          accessibilityRole="button"
          accessibilityLabel={demo.title}
          accessibilityHint={demo.subtitle}
          style={({ pressed }) => [styles.card, pressed && styles.pressed]}
        >
          <IconBadge icon={demo.icon} color={demo.color} size={40} />
          <Text style={styles.title}>{demo.title}</Text>
          <Text style={styles.subtitle}>{demo.subtitle}</Text>
        </Pressable>
      ))}
    </TabScreen>
  );
};

export default DemoList;

const styles = StyleSheet.create({
  card: {
    borderRadius: layout.cardRadius,
    padding: layout.screenPadding,
    marginBottom: 12,
    backgroundColor: colors.surface,
  },
  pressed: {
    backgroundColor: colors.surfacePressed,
  },
  title: {
    fontSize: 20,
    fontWeight: '700',
    color: colors.textPrimary,
    marginTop: 12,
  },
  subtitle: {
    fontSize: 15,
    lineHeight: 20,
    color: colors.textSecondary,
    marginTop: 4,
  },
});
