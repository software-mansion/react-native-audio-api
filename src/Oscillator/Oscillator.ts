import type { OscillatorNode } from './types';
import {
  getAudioContextModule,
  installModule,
  verifyExpoGo,
  verifyOnDevice,
} from '../utils/install';

function verifyInstallation() {
  if (global.__OscillatorProxy == null)
    throw new Error(
      'Failed to install react-native-audio-context, the native initializer function does not exist. Are you trying to use Oscillator from different JS Runtimes?'
    );
}

function createOscillatorProxy(): (frequency: number) => OscillatorNode {
  if (global.__OscillatorProxy) {
    return global.__OscillatorProxy;
  }

  verifyExpoGo();

  const AudioContextModule = getAudioContextModule();

  verifyOnDevice(AudioContextModule);
  installModule(AudioContextModule);
  verifyInstallation();

  if (global.__OscillatorProxy == null) {
    throw new Error('Failed to initialize __OscillatorProxy.');
  }

  return global.__OscillatorProxy;
}

// Call the creator and export what it returns
export default createOscillatorProxy();
