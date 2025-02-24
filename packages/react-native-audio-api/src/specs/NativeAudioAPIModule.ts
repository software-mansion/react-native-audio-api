import { TurboModuleRegistry, TurboModule } from 'react-native';

interface Spec extends TurboModule {
  install(): boolean;
}

export default TurboModuleRegistry.get<Spec>('AudioAPIModule');
