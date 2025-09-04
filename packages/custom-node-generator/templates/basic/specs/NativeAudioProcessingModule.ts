import { TurboModule, TurboModuleRegistry } from 'react-native';

export interface Spec extends TurboModule {
    injectCustomProcessorInstaller(): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>("NativeAudioProcessingModule");