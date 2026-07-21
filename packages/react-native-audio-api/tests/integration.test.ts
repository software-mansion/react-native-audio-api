/* eslint-disable */

import * as MockAPI from 'react-native-audio-api/mock';

describe('Mock Integration Tests', () => {
  describe('Audio Graph Creation', () => {
    it('should create a complete audio graph with oscillator, gain, and destination', () => {
      const context = new MockAPI.AudioContext();

      // Create nodes
      const oscillator = context.createOscillator();
      const gainNode = context.createGain();

      // Configure oscillator
      oscillator.type = 'sine';
      oscillator.frequency.value = 440;

      // Configure gain
      gainNode.gain.value = 0.5;

      // Connect the graph
      oscillator.connect(gainNode);
      gainNode.connect(context.destination);

      // Verify the setup
      expect(oscillator.type).toBe('sine');
      expect(oscillator.frequency.value).toBe(440);
      expect(gainNode.gain.value).toBe(0.5);
      expect(context.destination.numberOfOutputs).toBe(1);
    });

    it('should expose an AudioListener with nine spatialization params', () => {
      const context = new MockAPI.AudioContext();
      const listener = context.listener;

      expect(listener).toBeDefined();

      // Spec defaults: position (0, 0, 0), forward (0, 0, -1), up (0, 1, 0)
      expect(listener.positionX.value).toBe(0);
      expect(listener.positionY.value).toBe(0);
      expect(listener.positionZ.value).toBe(0);
      expect(listener.forwardX.value).toBe(0);
      expect(listener.forwardY.value).toBe(0);
      expect(listener.forwardZ.value).toBe(-1);
      expect(listener.upX.value).toBe(0);
      expect(listener.upY.value).toBe(1);
      expect(listener.upZ.value).toBe(0);

      // Params accept direct writes and automation
      listener.positionX.value = 5;
      expect(listener.positionX.value).toBe(5);
      listener.positionY.setValueAtTime(2, 0);
      expect(listener.positionY.value).toBe(2);
    });

    it('should create an audio graph with effects chain', () => {
      const context = new MockAPI.AudioContext();

      const oscillator = context.createOscillator();
      const filter = context.createBiquadFilter();
      const delay = context.createDelay();
      const gainNode = context.createGain();

      // Configure effects
      filter.type = 'lowpass';
      filter.frequency.value = 2000;
      delay.delayTime.value = 0.3;
      gainNode.gain.value = 0.8;

      // Create effects chain
      oscillator.connect(filter);
      filter.connect(delay);
      delay.connect(gainNode);
      gainNode.connect(context.destination);

      expect(filter.type).toBe('lowpass');
      expect(delay.delayTime.value).toBe(0.3);
    });
  });

  describe('Audio Recording Workflow', () => {
    it('should set up recording with adapter node', async () => {
      const context = new MockAPI.AudioContext();
      const recorder = new MockAPI.AudioRecorder();

      // Configure file output
      const configResult = recorder.enableFileOutput({
        format: MockAPI.FileFormat.M4A,
        channelCount: 2,
        directory: MockAPI.FileDirectory.Document,
      });

      expect(configResult.status).toBe('success');

      // Create audio source
      const oscillator = context.createOscillator();
      const recorderAdapter = context.createRecorderAdapter();

      // Connect to recorder
      oscillator.connect(recorderAdapter);
      recorder.connect(recorderAdapter);

      // Start recording
      const startResult = await recorder.start({
        fileNameOverride: 'test-recording.m4a',
      });
      expect(startResult.status).toBe('success');
      expect(recorder.isRecording()).toBe(true);

      // Stop recording
      const stopResult = await recorder.stop();
      expect(stopResult.status).toBe('success');
      // @ts-ignore - paths is not seen as the correct type
      expect(stopResult.paths?.length).toBeGreaterThan(0);
      expect(recorder.isRecording()).toBe(false);
    });

    it('should handle audio data callbacks during recording', () => {
      const recorder = new MockAPI.AudioRecorder();
      const audioDataCallback = jest.fn();

      const result = recorder.onAudioReady(
        {
          sampleRate: 44100,
          bufferLength: 1024,
          channelCount: 2,
        },
        audioDataCallback
      );

      expect(result.status).toBe('success');

      // Simulate clearing callback
      recorder.clearOnAudioReady();

      // Should not throw when clearing again
      expect(() => recorder.clearOnAudioReady()).not.toThrow();
    });
  });

  describe('Offline Audio Processing', () => {
    it('should render offline audio context', async () => {
      const offlineContext = new MockAPI.OfflineAudioContext({
        numberOfChannels: 2,
        length: 44100, // 1 second at 44.1kHz
        sampleRate: 44100,
      });

      // Create a simple tone
      const oscillator = offlineContext.createOscillator();
      oscillator.frequency.value = 440;
      oscillator.connect(offlineContext.destination);

      // Start rendering
      const renderedBuffer = await offlineContext.startRendering();

      expect(renderedBuffer).toBeInstanceOf(MockAPI.AudioBuffer);
      expect(renderedBuffer.length).toBe(44100);
      expect(renderedBuffer.sampleRate).toBe(44100);
      expect(renderedBuffer.numberOfChannels).toBe(2);
    });
  });

  describe('Buffer Queue Management', () => {
    it('should manage audio buffer queue for seamless playback', () => {
      const context = new MockAPI.AudioContext();
      const queueSource = context.createBufferQueueSource();

      // Create multiple buffers
      const buffer1 = context.createBuffer(2, 1024, 44100);
      const buffer2 = context.createBuffer(2, 1024, 44100);
      const buffer3 = context.createBuffer(2, 1024, 44100);

      // Enqueue buffers
      const id1 = queueSource.enqueueBuffer(buffer1);
      const id2 = queueSource.enqueueBuffer(buffer2);
      const id3 = queueSource.enqueueBuffer(buffer3);

      expect(typeof id1).toBe('string');
      expect(typeof id2).toBe('string');
      expect(typeof id3).toBe('string');
      expect(id1).not.toBe(id2);

      // Set up buffer ended callback
      const bufferEndedCallback = jest.fn();
      queueSource.onBufferEnded = bufferEndedCallback;

      // Dequeue specific buffer
      queueSource.dequeueBuffer(id2);

      // Clear all buffers
      queueSource.clearBuffers();

      // Connect and start playback
      queueSource.connect(context.destination);
      queueSource.start();
    });
  });

  describe('System Integration', () => {
    it('should integrate with system audio management', () => {
      // Get preferred sample rate
      const preferredRate = MockAPI.AudioManager.getDevicePreferredSampleRate();
      expect(preferredRate).toBe(44100);

      // Set up volume monitoring
      const volumeCallback = jest.fn();
      const listener = MockAPI.AudioManager.addSystemEventListener(
        'volumeChange',
        volumeCallback
      );

      MockAPI.AudioManager.observeVolumeChanges(true);

      // Simulate volume change
      MockAPI.setMockSystemVolume(0.7);
      const currentVolume = MockAPI.useSystemVolume();
      expect(currentVolume).toBe(0.7);

      // Clean up
      MockAPI.AudioManager.observeVolumeChanges(false);
      listener.remove();
    });

    it('should create and manage notifications', () => {
      const playbackNotification = MockAPI.PlaybackNotificationManager.create({
        title: 'Test Song',
        artist: 'Test Artist',
        artwork: 'https://example.com/artwork.jpg',
      });

      expect(playbackNotification.update).toBeDefined();
      expect(playbackNotification.destroy).toBeDefined();

      // Update notification
      playbackNotification.update();

      // Clean up
      playbackNotification.destroy();
    });
  });

  describe('Error Handling', () => {
    it('should handle various error conditions', () => {
      // Test NotSupportedError
      expect(() => {
        throw new MockAPI.NotSupportedError('Feature not supported');
      }).toThrow('Feature not supported');

      // Test InvalidStateError
      expect(() => {
        throw new MockAPI.InvalidStateError('Invalid state');
      }).toThrow('Invalid state');

      // Test recorder connection errors
      const recorder = new MockAPI.AudioRecorder();
      const context = new MockAPI.AudioContext();
      const adapter = context.createRecorderAdapter();

      // First connection should work
      recorder.connect(adapter);

      // Second connection should throw
      expect(() => recorder.connect(adapter)).toThrow();
    });
  });

  describe('File Presets and Configuration', () => {
    it('should use different quality presets', () => {
      const recorder = new MockAPI.AudioRecorder();

      // Test with different presets
      const lowQualityConfig = {
        preset: MockAPI.FilePreset.Low,
        format: MockAPI.FileFormat.M4A,
      };

      const highQualityConfig = {
        preset: MockAPI.FilePreset.High,
        format: MockAPI.FileFormat.Wav,
      };

      const losslessConfig = {
        preset: MockAPI.FilePreset.Lossless,
        format: MockAPI.FileFormat.Wav,
      };

      // Configure with different presets
      recorder.enableFileOutput(lowQualityConfig);
      expect(recorder.options?.format).toBe(MockAPI.FileFormat.M4A);

      recorder.disableFileOutput();
      recorder.enableFileOutput(highQualityConfig);
      expect(recorder.options?.format).toBe(MockAPI.FileFormat.Wav);

      recorder.disableFileOutput();
      recorder.enableFileOutput(losslessConfig);
      expect(recorder.options?.format).toBe(MockAPI.FileFormat.Wav);
    });
  });
});
