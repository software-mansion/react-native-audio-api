# Mock Tests for React Native Audio API

This directory contains comprehensive test suites for validating the mock implementations of the react-native-audio-api library.

## Test Files

### `mock.test.ts`

Core unit tests that validate individual mock classes and their behavior:

- AudioContext and OfflineAudioContext functionality
- Audio node creation and configuration
- AudioRecorder state management and workflows
- Custom nodes (RecorderAdapter, BufferQueue, Streamer, Worklet nodes)
- Utility functions (decoding, processing)
- System classes (AudioManager, NotificationManagers)
- Error handling and type correctness

### `integration.test.ts`

Integration tests demonstrating real-world usage scenarios:

- Complete audio graph creation and connection
- Recording workflows with adapter nodes
- Offline audio processing and rendering
- Worklet-based custom processing
- Buffer queue management for seamless playback
- Audio streaming setup
- System integration (volume monitoring, notifications)
- Error condition handling
- File presets and configuration

### `setup.ts`

Jest setup file that configures the test environment:

- Mock global objects required by the audio API
- Console output suppression for cleaner test output
- Test lifecycle management

## Running Tests

### Prerequisites

Make sure you have Jest installed in your project:

```bash
npm install --save-dev jest @types/jest
# or
yarn add --dev jest @types/jest
```

### Run All Tests

```bash
# From the package root directory
npx jest --config __tests__/jest.config.json

# Or add to your package.json scripts:
# "test:mocks": "jest --config __tests__/jest.config.json"
npm run test:mocks
```

### Run Specific Test Files

```bash
# Run only unit tests
npx jest __tests__/mock.test.ts

# Run only integration tests
npx jest __tests__/integration.test.ts
```

### Run with Coverage

```bash
npx jest --config __tests__/jest.config.json --coverage
```

### Watch Mode for Development

```bash
npx jest --config __tests__/jest.config.json --watch
```

## Test Coverage

The tests cover:

✅ **Core Audio API**

- AudioContext lifecycle management
- Audio node creation and connection
- AudioParam manipulation
- Buffer management

✅ **Custom React Native Extensions**

- AudioRecorder with file output and callbacks
- RecorderAdapterNode connection management
- AudioBufferQueueSourceNode for seamless playback
- StreamerNode for network audio streams
- Worklet nodes for custom processing

✅ **System Integration**

- AudioManager for device interaction
- Notification managers for media controls
- Volume monitoring and system events
- Hook integration (useSystemVolume)

✅ **Error Handling**

- Custom error classes
- Proper error conditions and validation
- Connection state management

✅ **Type Safety**

- All mock implementations use proper TypeScript types
- No `any` or `object` types used
- Proper interface compliance

## Usage in Your Tests

### Basic Mock Setup

```typescript
import * as MockAPI from '../src/mock';

// Replace the entire module
jest.mock('react-native-audio-api', () => require('./path/to/mock'));

// Or import mocks directly
const context = new MockAPI.AudioContext();
const recorder = new MockAPI.AudioRecorder();
```

### Testing Audio Workflows

```typescript
describe('Audio Processing', () => {
  it('should create and connect audio nodes', () => {
    const context = new MockAPI.AudioContext();
    const oscillator = context.createOscillator();
    const gain = context.createGain();

    oscillator.connect(gain);
    gain.connect(context.destination);

    expect(oscillator.frequency.value).toBe(440);
    expect(gain.gain.value).toBe(1);
  });
});
```

### Testing Recording

```typescript
describe('Audio Recording', () => {
  it('should handle recording workflow', () => {
    const recorder = new MockAPI.AudioRecorder();

    recorder.enableFileOutput({ format: MockAPI.FileFormat.M4A });
    recorder.start();
    expect(recorder.isRecording()).toBe(true);

    const result = recorder.stop();
    expect(result.status).toBe('success');
  });
});
```

## Contributing

When adding new features to the mock implementations:

1. Add corresponding unit tests in `mock.test.ts`
2. Add integration examples in `integration.test.ts`
3. Ensure all tests pass and maintain high coverage
4. Follow the existing test patterns and naming conventions

## Notes

- Tests run in Node.js environment, not React Native
- Global objects are mocked in setup.ts
- Console output is suppressed during tests for cleaner output
- All async operations use proper Promise patterns
- Mock implementations maintain state correctly for testing scenarios
