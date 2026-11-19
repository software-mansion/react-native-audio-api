import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import type { FC } from 'react';
import React from 'react';
import { nativeNanoSymbol } from 'react-native-nano-icons/symbols';

import { exampleCategories, Examples } from '../examples';
import { otherScreens } from '../other';
import DemoList from '../screens/DemoList';
import ExampleList, { type ExampleListSection } from '../screens/ExampleList';
import { colors } from '../styles';

type MainTabsParamList = {
  Tests: undefined;
  DemoApps: undefined;
  Other: undefined;
};

export const MainTabs = createBottomTabNavigator<MainTabsParamList>();

const testsSections: ExampleListSection[] = exampleCategories.map(
  (category) => ({
    key: category.key,
    title: category.title,
    color: category.color,
    examples: Examples.filter((example) => example.category === category.key),
  })
);

const otherSections: ExampleListSection[] = [
  { key: 'other', color: colors.navy, examples: otherScreens },
];

const TestsTab: FC = () => (
  <ExampleList title="Tests" sections={testsSections} />
);
const OtherTab: FC = () => (
  <ExampleList title="Other" sections={otherSections} />
);

const MainTabsNavigator: FC = () => (
  <MainTabs.Navigator
    screenOptions={{
      tabBarActiveTintColor: colors.accent,
      tabBarInactiveTintColor: colors.border,
      tabBarStyle: { backgroundColor: colors.background },
    }}
  >
    <MainTabs.Screen
      name="Tests"
      component={TestsTab}
      options={{
        title: 'Tests',
        tabBarIcon: () => nativeNanoSymbol('list-check'),
      }}
    />
    <MainTabs.Screen
      name="DemoApps"
      component={DemoList}
      options={{
        title: 'Demo Apps',
        tabBarIcon: () => nativeNanoSymbol('gauge'),
      }}
    />
    <MainTabs.Screen
      name="Other"
      component={OtherTab}
      options={{
        title: 'Other',
        tabBarIcon: () => nativeNanoSymbol('waves'),
      }}
    />
  </MainTabs.Navigator>
);

export default MainTabsNavigator;
