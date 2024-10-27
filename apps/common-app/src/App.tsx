import React from 'react';
import type { FC } from 'react';
import Animated from 'react-native-reanimated';
import { createStackNavigator } from '@react-navigation/stack';
import { FlatList, StyleSheet, Text, Pressable } from 'react-native';
import { GestureHandlerRootView } from 'react-native-gesture-handler';
import { NavigationContainer, useNavigation } from '@react-navigation/native';

import Container from './components/Container';
import { Examples, MainStackProps } from './examples';
import { layout, colors } from './styles';

const Stack = createStackNavigator();

Animated.addWhitelistedNativeProps({ text: true });

const HomeScreen: FC = () => {
  const navigation = useNavigation<MainStackProps>();

  return (
    <Container centered={false}>
      <FlatList
        contentContainerStyle={styles.scrollView}
        data={Examples}
        renderItem={({ item }) => (
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
        )}
        keyExtractor={(item) => item.key}
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
            headerStyle: {
              backgroundColor: colors.main,
            },
            headerTintColor: '#fff',
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
    paddingVertical: layout.spacing * 2,
    paddingHorizontal: layout.spacing * 2,
    marginBottom: layout.spacing,
    borderWidth: 2,
    borderColor: colors.border,
    borderRadius: layout.radius,
  },
  scrollView: {
    padding: layout.spacing * 2,
  },
});

export default App;
