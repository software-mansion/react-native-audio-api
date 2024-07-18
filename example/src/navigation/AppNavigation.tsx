import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';
import SimplePlayer from '../examples/SimplePlayer';

const Tab = createBottomTabNavigator();

function AppNavigation() {
  return (
    <NavigationContainer>
      <Tab.Navigator>
        <Tab.Screen name="Simple Player" component={SimplePlayer} />
      </Tab.Navigator>
    </NavigationContainer>
  );
}

export default AppNavigation;
