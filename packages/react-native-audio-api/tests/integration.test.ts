/* eslint-disable */

import * as MockAPI from '../src/mock';

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
      expect(context.destination.numberOfOutputs).toBe(0);
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
    it('should set up recording with adapter node', () => {
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
      const startResult = recorder.start({
        fileNameOverride: 'test-recording.m4a',
      });
      expect(startResult.status).toBe('success');
      expect(recorder.isRecording()).toBe(true);

      // Stop recording
      const stopResult = recorder.stop();
      expect(stopResult.status).toBe('success');
      expect((stopResult as { path?: string }).path).toBeDefined();
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

  describe('Worklet Processing', () => {
    it('should create and use worklet nodes for custom processing', () => {
      const context = new MockAPI.AudioContext();

      const processingCallback = jest.fn(
        (inputData, outputData, framesToProcess) => {
          // Mock audio processing logic
          for (let channel = 0; channel < outputData.length; channel++) {
            for (let i = 0; i < framesToProcess; i++) {
              outputData[channel][i] = inputData[channel][i] * 0.5; // Simple gain
            }
          }
        }
      );

      const workletNode = new MockAPI.WorkletProcessingNode(
        context,
        'AudioRuntime',
        processingCallback
      );

      expect(workletNode).toBeInstanceOf(MockAPI.WorkletProcessingNode);
      expect(workletNode.context).toBe(context);
    });

    it('should create worklet source node for custom audio generation', () => {
      const context = new MockAPI.AudioContext();

      const sourceCallback = jest.fn(
        (audioData, framesToProcess, currentTime) => {
          // Mock audio generation logic
          for (let channel = 0; channel < audioData.length; channel++) {
            for (let i = 0; i < framesToProcess; i++) {
              audioData[channel][i] = Math.sin(currentTime + i); // Simple sine wave
            }
          }
        }
      );

      const workletSource = new MockAPI.WorkletSourceNode(
        context,
        'UIRuntime',
        sourceCallback
      );

      expect(workletSource).toBeInstanceOf(MockAPI.WorkletSourceNode);
      expect(workletSource.context).toBe(context);
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

  describe('Streaming Audio', () => {
    it('should set up audio streaming node', () => {
      const context = new MockAPI.AudioContext();

      // Create streamer with options
      const streamer = context.createStreamer({
        streamPath: 'https://example.com/audio-stream',
      });

      expect(streamer.streamPath).toBe('https://example.com/audio-stream');

      // Connect to output
      streamer.connect(context.destination);

      // Control playback
      streamer.start();
      streamer.pause();
      streamer.resume();
      streamer.stop();
    });

    it('should handle streamer initialization errors', () => {
      const context = new MockAPI.AudioContext();
      const streamer = context.createStreamer();

      // First initialization should succeed
      expect(streamer.initialize('http://stream1.com')).toBe(true);

      // Second initialization should fail
      expect(() => streamer.initialize('http://stream2.com')).toThrow();
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
