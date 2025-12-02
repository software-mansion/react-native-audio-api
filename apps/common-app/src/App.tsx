import { NavigationContainer, useNavigation } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';
import type { FC } from 'react';
import React from 'react';
import {
  FlatList,
  ListRenderItem,
  Pressable,
  StyleSheet,
  Text,
} from 'react-native';
import { GestureHandlerRootView } from 'react-native-gesture-handler';

import { Spacer } from './components';
import Container from './components/Container';
import { Example, Examples, MainStackProps } from './examples';
import { colors, layout } from './styles';

const Stack = createStackNavigator();

const ItemSeparatorComponent = () => <Spacer.Vertical size={16} />;

const HomeScreen: FC = () => {
  const navigation = useNavigation<MainStackProps>();

  const renderItem: ListRenderItem<Example> = ({ item }) => (
    <Pressable
      onPress={() => navigation.navigate(item.key)}
      key={item.key}
      style={({ pressed }) => [
        styles.button,
        { borderStyle: pressed ? 'solid' : 'dashed' },
      ]}
    >
      <Text style={styles.title}>{item.title}</Text>
      <Text style={styles.subtitle}>{item.subtitle}</Text>
    </Pressable>
  );

  return (
    <Container>
      <FlatList
        data={Examples}
        renderItem={renderItem}
        keyExtractor={(item) => item.key}
        contentContainerStyle={styles.scrollView}
        ItemSeparatorComponent={ItemSeparatorComponent}
      />
    </Container>
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
            headerBackTitle: ' ',
            headerBackAccessibilityLabel: 'Go back',
          }}
        >
          <Stack.Screen
            name="Home"
            component={HomeScreen}
            options={{ title: 'Audio API examples' }}
          />
          {Examples.map((item) => (
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

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  title: {
    fontSize: 24,
    fontWeight: '700',
    color: colors.white,
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
  },
  scrollView: {
    padding: layout.spacing * 2,
  },
});

export default App;
