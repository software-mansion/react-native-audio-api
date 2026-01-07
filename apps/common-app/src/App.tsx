import { NavigationContainer, useNavigation } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import { Gauge, ListCheck, Waves } from 'lucide-react-native';
import type { FC } from 'react';
import React from 'react';
import {
  FlatList,
  ListRenderItem,
  Pressable,
  StyleSheet,
  Text,
  View,
} from 'react-native';
import { GestureHandlerRootView } from 'react-native-gesture-handler';

import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import { Spacer } from './components';
import Container from './components/Container';
import { demos, DemoScreen } from './demos';
import { Example, Examples, MainStackProps } from './examples';
import { colors, layout } from './styles';

const Stack = createStackNavigator();

const TestsScreen: FC = () => {
  const navigation = useNavigation<MainStackProps>();

  const renderItem: ListRenderItem<Example> = ({
    item: { Icon, key, title },
  }) => (
    <Pressable
      key={key}
      style={styles.buttonSmall}
      onPress={() => navigation.navigate(key)}
    >
      <Icon color={colors.white} size={18} />
      <Text style={styles.titleSmall}>{title}</Text>
    </Pressable>
  );

  return (
    <Container headless>
      <FlatList
        data={Examples}
        renderItem={renderItem}
        keyExtractor={(item) => item.key}
        contentContainerStyle={styles.scrollView}
        ItemSeparatorComponent={ItemSeparatorComponentSmall}
        numColumns={2}
      />
    </Container>
  );
};

const DemoAppsScreen: FC = () => {
  const navigation = useNavigation<MainStackProps>();

  const renderItem: ListRenderItem<DemoScreen> = ({ item }) => (
    <Pressable
      onPress={() => navigation.navigate(item.key)}
      key={item.key}
      style={({ pressed }) => [
        styles.button,
        { borderStyle: pressed ? 'solid' : 'dashed' },
      ]}
    >
      <item.icon color={colors.white} size={24} />
      <View style={styles.buttonInner}>
        <Text style={styles.title}>{item.title}</Text>
        <Text style={styles.subtitle}>{item.subtitle}</Text>
      </View>
    </Pressable>
  );

  return (
    <Container headless>
      <FlatList
        data={demos}
        renderItem={renderItem}
        keyExtractor={(item) => item.key}
        contentContainerStyle={styles.scrollView}
        ItemSeparatorComponent={ItemSeparatorComponent}
      />
    </Container>
  );
};

const OtherScreen: FC = () => {
  return <Container headless />;
};

const MainTabs = createBottomTabNavigator<MainStackProps>({
  screens: {
    Tests: TestsScreen,
    DemoApps: DemoAppsScreen,
    Other: OtherScreen,
  },
});

const tabBarIcon = ({
  routeName,
  color,
  size,
}: {
  routeName: string;
  color: string;
  size: number;
}) => {
  if (routeName === 'Tests') {
    return <ListCheck color={color} size={size} />;
  } else if (routeName === 'DemoApps') {
    return <Gauge color={color} size={size} />;
  } else if (routeName === 'Other') {
    return <Waves color={color} size={size} />;
  }
  return null;
};

const MainTabsScreen: FC = () => {
  return (
    <MainTabs.Navigator
      screenOptions={({ route }: { route: { name: string } }) => ({
        tabBarIcon: ({ color, size }: { color: string; size: number }) =>
          tabBarIcon({ routeName: route.name, color, size }),
        tabBarActiveTintColor: '#ff7774',
        tabBarInactiveTintColor: colors.border,
        tabBarTransparent: true,
        tabBarStyle: {
          backgroundColor: colors.background,
        },
        headerShown: false,
      })}
    >
      <MainTabs.Screen
        name="Tests"
        component={TestsScreen}
        options={{
          title: 'Tests',
        }}
      />
      <MainTabs.Screen
        name="DemoApps"
        component={DemoAppsScreen}
        options={{
          title: 'Demo Apps',
        }}
      />
      <MainTabs.Screen
        name="Other"
        component={OtherScreen}
        options={{
          title: 'Other',
        }}
      />
    </MainTabs.Navigator>
  );
};

const App: FC = () => {
  return (
    <GestureHandlerRootView style={styles.container}>
      <NavigationContainer>
        <Stack.Navigator
          screenOptions={{
            headerShown: true,
            headerTransparent: true,
            headerStyle: {
              backgroundColor: 'transparent',
            },
            headerTintColor: colors.white,
            headerBackTitle: 'Back',
            headerBackAccessibilityLabel: 'Go back',
          }}>
          <Stack.Screen
            name="MainTabs"
            component={MainTabsScreen}
            options={{ headerShown: false }}
          />
          {Examples.map((item) => (
            <Stack.Screen
              key={item.key}
              name={item.key}
              component={item.screen}
              options={{ title: item.title }}
            />
          ))}
          {demos.map((item) => (
            <Stack.Screen
              key={item.key}
              name={item.key}
              component={item.screen}
              options={{ title: item.title }}
            />
          ))}
        </Stack.Navigator>
      </NavigationContainer>
    </GestureHandlerRootView>
  );
};

export default App;

const ItemSeparatorComponent = () => <Spacer.Vertical size={16} />;

const ItemSeparatorComponentSmall = () => (
  <View>
    <Spacer.Vertical size={6} />
    <View style={styles.hr} />
    <Spacer.Vertical size={6} />
  </View>
);

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  title: {
    fontSize: 24,
    fontWeight: '700',
    color: colors.white,
    lineHeight: 24,
  },
  subtitle: {
    opacity: 0.6,
    color: colors.white,
  },
  button: {
    borderWidth: 2,
    borderColor: colors.border,
    borderRadius: layout.radius,
    paddingVertical: layout.spacing * 2,
    paddingHorizontal: layout.spacing * 2,
    flexDirection: 'row',
    alignItems: 'flex-start',
    gap: layout.spacing,
  },
  buttonInner: {
    flexShrink: 1,
  },
  buttonSmall: {
    padding: layout.spacing,
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
    width: '48%',
    margin: '1%',
    height: 40,
    borderWidth: StyleSheet.hairlineWidth,
    borderColor: colors.white,
  },
  titleSmall: {
    fontSize: 16,
    fontWeight: '600',
    color: colors.white,
    marginLeft: 4,
    flexShrink: 1,
  },
  subtitleSmall: {
    opacity: 0.6,
    color: colors.white,
  },
  scrollView: {},
  hr: {
    // height: StyleSheet.hairlineWidth,
    // // backgroundColor: colors.border,
    // marginLeft: 60,
    // marginRight: '35%',
  },
  row: {
    flexDirection: 'row',
    alignItems: 'center',
    gap: layout.spacing,
  },
});
