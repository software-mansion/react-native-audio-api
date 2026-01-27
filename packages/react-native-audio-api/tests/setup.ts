// __tests__/setup.ts
import type { IAudioEventEmitter } from '../src/interfaces';

// Mock global objects that might be needed
global.createAudioContext = jest.fn();
global.createAudioRecorder = jest.fn();
global.AudioEventEmitter = {} as IAudioEventEmitter;

// Set up global test environment
beforeAll(() => {
  // Suppress console warnings for tests
  jest.spyOn(console, 'warn').mockImplementation(() => {});
  jest.spyOn(console, 'error').mockImplementation(() => {});
});

afterAll(() => {
  // Restore console methods
  (console.warn as jest.Mock).mockRestore();
  (console.error as jest.Mock).mockRestore();
});
