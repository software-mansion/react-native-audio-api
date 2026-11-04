import React, { PropsWithChildren } from 'react';
import { Platform, ScrollView, StyleSheet, Text } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import { SafeAreaView } from 'react-native-screens/experimental';

import { colors, layout } from '../styles';

type TabScreenProps = PropsWithChildren<{
  title: string;
}>;

const isAndroid = Platform.OS === 'android';

/**
 * Root of a bottom-tab screen: a scroll view with a large title. On iOS the
 * scroll view insets itself for the status bar and the floating glass tab bar,
 * so content scrolls underneath both. Android gets no automatic insets: the
 * content is padded by the status bar on top, and the scroll view ends above
 * the opaque tab bar. React Navigation's native tabs report a tab bar height of
 * 0, so the bottom edge comes from the tab container's interface insets.
 */
const TabScreen: React.FC<TabScreenProps> = ({ title, children }) => {
  const insets = useSafeAreaInsets();
  const androidTopInset = isAndroid ? insets.top : 0;

  const scrollView = (
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

  if (!isAndroid) {
    return scrollView;
  }

  return (
    <SafeAreaView
      edges={{ bottom: true }}
      insetType="interface"
      style={styles.screen}
    >
      {scrollView}
    </SafeAreaView>
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
