import React from 'react';
import AppContainer from './components/AppContainer';
import AppNavigation from './navigation/AppNavigation';
import { SafeAreaProvider } from 'react-native-safe-area-context';

const App = () => {
  return (
    <SafeAreaProvider>
      <AppContainer>
        <AppNavigation />
      </AppContainer>
    </SafeAreaProvider>
  );
};

export default App;
