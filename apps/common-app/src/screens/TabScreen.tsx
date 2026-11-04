import React, { PropsWithChildren } from 'react';
import { Platform, ScrollView, StyleSheet, Text } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';

import { colors, layout } from '../styles';

type TabScreenProps = PropsWithChildren<{
  title: string;
}>;

/**
 * Root of a bottom-tab screen: a scroll view with a large title. On iOS the
 * scroll view insets itself for the status bar and the floating glass tab bar,
 * so content scrolls underneath both.
 */
const TabScreen: React.FC<TabScreenProps> = ({ title, children }) => {
  const insets = useSafeAreaInsets();
  const androidTopInset = Platform.OS === 'android' ? insets.top : 0;

  return (
    <ScrollView
      style={styles.screen}
      contentInsetAdjustmentBehavior="automatic"
      contentContainerStyle={[
        styles.content,
        { paddingTop: androidTopInset + layout.spacing },
      ]}
    >
      <Text style={styles.largeTitle} accessibilityRole="header">
        {title}
      </Text>
      {children}
    </ScrollView>
  );
};

export default TabScreen;

const styles = StyleSheet.create({
  screen: {
    flex: 1,
    backgroundColor: colors.background,
  },
  content: {
    paddingHorizontal: layout.screenPadding,
    paddingBottom: layout.spacing * 3,
  },
  largeTitle: {
    fontSize: 34,
    fontWeight: '700',
    letterSpacing: -0.4,
    color: colors.textPrimary,
    marginHorizontal: 4,
    marginBottom: 20,
  },
});
