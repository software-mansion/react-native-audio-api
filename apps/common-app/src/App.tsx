import React from 'react';
import type { FC } from 'react';
import { ScrollView, StyleSheet, Text, TouchableOpacity } from 'react-native';
import { NavigationContainer, useNavigation } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import Container from './components/Container';
import { Examples } from './examples';
import { layout, colors } from './styles';

const Stack = createStackNavigator();

const HomeScreen: FC = () => {
  const navigation = useNavigation();

  return (
    <Container centered={false}>
      <ScrollView contentContainerStyle={styles.scrollView}>
        {Object.entries(Examples).map(([key, example]) => (
          <TouchableOpacity
            onPress={() => navigation.navigate(key as never)}
            key={key}
            style={styles.button}
          >
            <Text style={styles.title}>{example.title}</Text>
            <Text style={styles.subtitle}>{example.subtitle}</Text>
          </TouchableOpacity>
        ))}
      </ScrollView>
    </Container>
  );
};

const App: FC = () => {
  return (
    <NavigationContainer>
      <Stack.Navigator
        screenOptions={{
          headerStyle: {
            backgroundColor: colors.darkblue,
          },
          headerTintColor: colors.white,
        }}
      >
        <Stack.Screen
          name="Home"
          component={HomeScreen}
          options={{ title: 'Audio API examples' }}
        />
        {Object.keys(Examples).map((key) => (
          <Stack.Screen
            key={key}
            name={key}
            component={Examples[key].screen}
            options={{ title: Examples[key].title }}
          />
        ))}
      </Stack.Navigator>
    </NavigationContainer>
  );
};

const styles = StyleSheet.create({
  title: {
    fontSize: 24,
    fontWeight: '700',
  },
  subtitle: {
    opacity: 0.6,
  },
  button: {
    paddingVertical: layout.spacing * 2,
    paddingHorizontal: layout.spacing * 2,
    marginBottom: layout.spacing,
    borderWidth: 2,
    borderColor: colors.border,
    borderStyle: 'dashed',
    borderRadius: layout.radius,
  },
  scrollView: {
    padding: layout.spacing * 2,
  },
});

export default App;
