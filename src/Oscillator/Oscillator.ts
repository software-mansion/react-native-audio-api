import type { OscillatorNode } from './types';
import {
  getAudioContextModule,
  installModule,
  verifyExpoGo,
  verifyOnDevice,
} from '../utils/install';

function verifyInstallation() {
  if (global.createOscillator == null)
    throw new Error(
      'Failed to install react-native-audio-context, the native initializer function does not exist. Are you trying to use Oscillator from different JS Runtimes?'
    );
}

function createOscillator(): (frequency: number) => OscillatorNode {
  if (global.createOscillator) {
    return global.createOscillator;
  }

  verifyExpoGo();

  const AudioContextModule = getAudioContextModule();

  verifyOnDevice(AudioContextModule);
  installModule(AudioContextModule);
  verifyInstallation();

  if (global.createOscillator == null) {
    throw new Error('Failed to initialize createOscillator.');
  }

  return global.createOscillator;
}

// Call the creator and export what it returns
export default createOscillator();
