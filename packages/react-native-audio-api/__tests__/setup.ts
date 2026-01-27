/**
 * Jest setup file for mock tests
 */

// Mock global objects that might be needed
global.createAudioContext = jest.fn();
global.createAudioRecorder = jest.fn();
global.AudioEventEmitter = {};

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
