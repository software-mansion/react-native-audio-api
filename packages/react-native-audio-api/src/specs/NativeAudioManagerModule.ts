import { TurboModuleRegistry, TurboModule } from 'react-native';
import { Float } from 'react-native/Libraries/Types/CodegenTypes';

interface Spec extends TurboModule {
  setNowPlaying(info: {
    [key: string]: string | boolean | number | undefined;
  }): void;
  setSessionCategory(category: string): void;
  setSessionMode(mode: string): void;
  setSessionCategoryOptions(options: Array<string>): void;

  getSampleRate(): Float;
}

export default TurboModuleRegistry.getEnforcing<Spec>('AudioManagerModule');
