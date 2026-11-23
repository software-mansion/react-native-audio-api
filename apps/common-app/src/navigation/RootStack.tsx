import type { NavigatorScreenParams } from '@react-navigation/native';
import { createNativeStackNavigator } from '@react-navigation/native-stack';
import type { FC } from 'react';
import React from 'react';
import { Platform } from 'react-native';

import { demos, DemoKey } from '../demos';
import { ExampleKey, Examples } from '../examples';
import { otherScreens } from '../other';
import { colors } from '../styles';
import MainTabsNavigator, { MainTabs } from './MainTabs';

export type RootStackParamList = {
  MainTabs: NavigatorScreenParams<typeof MainTabs>;
} & Record<ExampleKey | DemoKey, undefined>;

const RootStack = createNativeStackNavigator<RootStackParamList>();

type RootStackType = typeof RootStack;

declare module '@react-navigation/native' {
  interface RootNavigator extends RootStackType {}
}

const detailScreens = [...Examples, ...otherScreens, ...demos];

const RootStackNavigator: FC = () => (
  <RootStack.Navigator
    screenOptions={{
      headerTransparent: true,
      headerTintColor: colors.textPrimary,
      headerBackButtonDisplayMode: 'minimal',
      headerBackTitle: 'Back',
      contentStyle: { backgroundColor: colors.background },
    }}
  >
    <RootStack.Screen
      name="MainTabs"
      component={MainTabsNavigator}
      options={{ headerShown: Platform.OS === 'ios', title: '' }}
    />
    {detailScreens.map((item) => (
      <RootStack.Screen
        key={item.key}
        name={item.key}
        component={item.screen}
        options={{ title: item.title }}
      />
    ))}
  </RootStack.Navigator>
);

export default RootStackNavigator;
