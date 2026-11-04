import type { NavigatorScreenParams } from '@react-navigation/native';
import {
  createNativeStackNavigator,
  type NativeStackNavigationOptions,
} from '@react-navigation/native-stack';
import type { FC, ReactNode } from 'react';
import React from 'react';
import { Platform } from 'react-native';

import { DemoControlsKey, demos, DemoKey } from '../demos';
import { ExampleControlsKey, ExampleKey, Examples } from '../examples';
import { otherScreens } from '../other';
import { colors, layout } from '../styles';
import { ControlsSheetLayout } from './ControlsSheetLayout';
import MainTabsNavigator, { MainTabs } from './MainTabs';

type RootStackParamList = {
  MainTabs: NavigatorScreenParams<typeof MainTabs>;
} & Record<
  ExampleKey | ExampleControlsKey | DemoKey | DemoControlsKey,
  undefined
>;

const RootStack = createNativeStackNavigator<RootStackParamList>();

type RootStackType = typeof RootStack;

declare module '@react-navigation/native' {
  interface RootNavigator extends RootStackType {}
}

const detailScreens = [...Examples, ...otherScreens, ...demos];

const controlsSheets = [...Examples, ...demos].flatMap(({ key, controls }) => {
  if (!controls) {
    return [];
  }

  const name = `${key}Controls` as const;
  const sheetLayout = ({ children }: { children: ReactNode }) => (
    <ControlsSheetLayout sheet={name} owner={key}>
      {children}
    </ControlsSheetLayout>
  );

  return [{ name, component: controls, layout: sheetLayout }];
});

/**
 * A non-dismissable sheet sized to its content. Nothing behind it is dimmed, so
 * the screen underneath stays interactive. On iOS the transparent content lets
 * the native sheet material (liquid glass on iOS 26) show through. Android has
 * no sheet material: a transparent sheet shows its own elevation shadow through
 * itself, a lighter outline offset from the real edge, so it gets a surface.
 */
const controlsSheetOptions: NativeStackNavigationOptions = {
  presentation: 'formSheet',
  headerShown: false,
  gestureEnabled: false,
  sheetAllowedDetents: 'fitToContents',
  sheetLargestUndimmedDetentIndex: 'last',
  sheetGrabberVisible: false,
  ...Platform.select<NativeStackNavigationOptions>({
    android: {
      contentStyle: { backgroundColor: colors.recess },
      sheetCornerRadius: layout.cardRadius,
    },
    default: { contentStyle: { backgroundColor: 'transparent' } },
  }),
};

const RootStackNavigator: FC = () => (
  <RootStack.Navigator
    screenOptions={{
      headerTransparent: true,
      headerTintColor: colors.white,
      headerBackButtonDisplayMode: 'minimal',
      headerBackTitle: 'Back',
      contentStyle: { backgroundColor: colors.background },
    }}
  >
    <RootStack.Screen
      name="MainTabs"
      component={MainTabsNavigator}
      options={{ headerShown: false }}
    />
    {detailScreens.map((item) => (
      <RootStack.Screen
        key={item.key}
        name={item.key}
        component={item.screen}
        options={{ title: item.title }}
      />
    ))}
    {controlsSheets.map((sheet) => (
      <RootStack.Screen
        key={sheet.name}
        name={sheet.name}
        component={sheet.component}
        options={controlsSheetOptions}
        layout={sheet.layout}
      />
    ))}
  </RootStack.Navigator>
);

export default RootStackNavigator;
