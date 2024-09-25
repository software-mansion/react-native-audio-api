import React from 'react';
import type { FC } from 'react';
import { FlatList, StyleSheet, Text, TouchableOpacity } from 'react-native';
import { NavigationContainer, useNavigation } from '@react-navigation/native';
import { createStackNavigator } from '@react-navigation/stack';

import Container from './components/Container';
import { Examples, MainStackProps } from './examples';
import { layout, colors } from './styles';

const Stack = createStackNavigator();

const HomeScreen: FC = () => {
  const navigation = useNavigation<MainStackProps>();

  return (
    <Container centered={false}>
      <FlatList
        contentContainerStyle={styles.scrollView}
        data={Examples}
        renderItem={({ item }) => (
          <TouchableOpacity
            onPress={() => navigation.navigate(item.key)}
            key={item.key}
            style={styles.button}
          >
            <Text style={styles.title}>{item.title}</Text>
            <Text style={styles.subtitle}>{item.subtitle}</Text>
          </TouchableOpacity>
        )}
        keyExtractor={(item) => item.key}
      />
    </Container>
  );
};

const App: FC = () => {
  return (
    <NavigationContainer>
      <Stack.Navigator
        screenOptions={{
          headerStyle: {
            backgroundColor: colors.main,
          },
          headerTintColor: colors.white,
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
