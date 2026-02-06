/* eslint-disable */

import * as MockAPI from '../src/mock';

describe('React Native Audio API Mocks', () => {
  describe('AudioContext', () => {
    it('should create an AudioContext with default sample rate', () => {
      const context = new MockAPI.AudioContext();
      expect(context.sampleRate).toBe(44100);
      expect(context.currentTime).toBe(0);
      expect(context.state).toBe('running');
      expect(context.destination).toBeInstanceOf(MockAPI.AudioDestinationNode);
    });

    it('should create an AudioContext with custom sample rate', () => {
      const context = new MockAPI.AudioContext({ sampleRate: 48000 });
      expect(context.sampleRate).toBe(48000);
    });

    it('should support context state management', async () => {
      const context = new MockAPI.AudioContext();
      expect(context.state).toBe('running');

      await context.suspend();
      expect(context.state).toBe('suspended');

      await context.resume();
      expect(context.state).toBe('running');

      await context.close();
      expect(context.state).toBe('closed');
    });
  });

  describe('OfflineAudioContext', () => {
    it('should create an OfflineAudioContext with specified parameters', () => {
      const context = new MockAPI.OfflineAudioContext({
        numberOfChannels: 2,
        length: 44100,
        sampleRate: 44100,
      });

      expect(context.sampleRate).toBe(44100);
      expect(context.length).toBe(44100);
    });

    it('should start rendering and return an AudioBuffer', async () => {
      const context = new MockAPI.OfflineAudioContext({
        numberOfChannels: 2,
        length: 44100,
        sampleRate: 44100,
      });

      const buffer = await context.startRendering();
      expect(buffer).toBeInstanceOf(MockAPI.AudioBuffer);
      expect(buffer.numberOfChannels).toBe(2);
      expect(buffer.length).toBe(44100);
      expect(buffer.sampleRate).toBe(44100);
    });
  });

  describe('AudioBuffer', () => {
    it('should create an AudioBuffer with correct properties', () => {
      const buffer = new MockAPI.AudioBuffer({
        numberOfChannels: 2,
        length: 1024,
        sampleRate: 44100,
      });

      expect(buffer.numberOfChannels).toBe(2);
      expect(buffer.length).toBe(1024);
      expect(buffer.sampleRate).toBe(44100);
      expect(buffer.duration).toBeCloseTo(1024 / 44100);
    });

    it('should provide channel data', () => {
      const buffer = new MockAPI.AudioBuffer({
        numberOfChannels: 2,
        length: 1024,
        sampleRate: 44100,
      });

      const channelData = buffer.getChannelData(0);
      expect(channelData).toBeInstanceOf(Float32Array);
      expect(channelData.length).toBe(1024);
    });
  });

  describe('Audio Nodes', () => {
    let context: MockAPI.AudioContext;

    beforeEach(() => {
      context = new MockAPI.AudioContext();
    });

    describe('GainNode', () => {
      it('should create a GainNode with correct properties', () => {
        const gainNode = context.createGain();
        expect(gainNode).toBeInstanceOf(MockAPI.GainNode);
        expect(gainNode.gain).toBeInstanceOf(MockAPI.AudioParam);
        expect(gainNode.gain.value).toBe(1);
      });

      it('should allow gain parameter manipulation', () => {
        const gainNode = context.createGain();
        gainNode.gain.value = 0.5;
        expect(gainNode.gain.value).toBe(0.5);

        const result = gainNode.gain.setValueAtTime(0.8, 0);
        expect(result).toBe(gainNode.gain);
        expect(gainNode.gain.value).toBe(0.8);
      });
    });

    describe('OscillatorNode', () => {
      it('should create an OscillatorNode with correct properties', () => {
        const oscillator = context.createOscillator();
        expect(oscillator).toBeInstanceOf(MockAPI.OscillatorNode);
        expect(oscillator.frequency).toBeInstanceOf(MockAPI.AudioParam);
        expect(oscillator.frequency.value).toBe(440);
        expect(oscillator.type).toBe('sine');
      });

      it('should allow type changes', () => {
        const oscillator = context.createOscillator();
        oscillator.type = 'square';
        expect(oscillator.type).toBe('square');
      });

      it('should support start/stop methods', () => {
        const oscillator = context.createOscillator();
        expect(() => oscillator.start()).not.toThrow();
        expect(() => oscillator.stop()).not.toThrow();
      });

      it('should support onended callback', () => {
        const oscillator = context.createOscillator();
        const callback = jest.fn();
        oscillator.onended = callback;
        expect(oscillator.onended).toBe(callback);
      });
    });

    describe('AnalyserNode', () => {
      it('should create an AnalyserNode with correct properties', () => {
        const analyser = context.createAnalyser();
        expect(analyser).toBeInstanceOf(MockAPI.AnalyserNode);
        expect(analyser.fftSize).toBe(2048);
        expect(analyser.frequencyBinCount).toBe(1024);
      });
    });

    describe('BiquadFilterNode', () => {
      it('should create a BiquadFilterNode with correct properties', () => {
        const filter = context.createBiquadFilter();
        expect(filter).toBeInstanceOf(MockAPI.BiquadFilterNode);
        expect(filter.type).toBe('lowpass');
        expect(filter.frequency.value).toBe(350);
        expect(filter.Q.value).toBe(1);
      });
    });
  });

  describe('Custom Nodes', () => {
    let context: MockAPI.AudioContext;

    beforeEach(() => {
      context = new MockAPI.AudioContext();
    });

    describe('RecorderAdapterNode', () => {
      it('should create a RecorderAdapterNode', () => {
        const recorderAdapter = context.createRecorderAdapter();
        expect(recorderAdapter).toBeInstanceOf(MockAPI.RecorderAdapterNode);
        expect(recorderAdapter.wasConnected).toBe(false);
      });
    });

    describe('AudioBufferQueueSourceNode', () => {
      it('should create an AudioBufferQueueSourceNode', () => {
        const queueSource = context.createBufferQueueSource();
        expect(queueSource).toBeInstanceOf(MockAPI.AudioBufferQueueSourceNode);
      });

      it('should support buffer queueing operations', () => {
        const queueSource = context.createBufferQueueSource();
        const buffer = context.createBuffer(2, 1024, 44100);

        const bufferId = queueSource.enqueueBuffer(buffer);
        expect(typeof bufferId).toBe('string');
        expect(bufferId.length).toBeGreaterThan(0);

        expect(() => queueSource.dequeueBuffer(bufferId)).not.toThrow();
        expect(() => queueSource.clearBuffers()).not.toThrow();
      });

      it('should support onBufferEnded callback', () => {
        const queueSource = context.createBufferQueueSource();
        const callback = jest.fn();
        queueSource.onBufferEnded = callback;
        expect(queueSource.onBufferEnded).toBe(callback);
      });
    });

    describe('StreamerNode', () => {
      it('should create a StreamerNode', () => {
        const streamer = context.createStreamer();
        expect(streamer).toBeInstanceOf(MockAPI.StreamerNode);
      });

      it('should support initialization with stream path', () => {
        const streamer = context.createStreamer();
        const result = streamer.initialize('http://example.com/stream');
        expect(result).toBe(true);
        expect(streamer.streamPath).toBe('http://example.com/stream');
      });

      it('should throw error on duplicate initialization', () => {
        const streamer = context.createStreamer();
        streamer.initialize('http://example.com/stream');
        expect(() => streamer.initialize('http://example.com/other')).toThrow();
      });
    });

    describe('WorkletNodes', () => {
      it('should create WorkletNode', () => {
        const workletNode = new MockAPI.WorkletNode(
          context,
          'AudioRuntime',
          () => {},
          1024,
          2
        );
        expect(workletNode).toBeInstanceOf(MockAPI.WorkletNode);
      });

      it('should create WorkletProcessingNode', () => {
        const processingNode = new MockAPI.WorkletProcessingNode(
          context,
          'AudioRuntime',
          () => {}
        );
        expect(processingNode).toBeInstanceOf(MockAPI.WorkletProcessingNode);
      });

      it('should create WorkletSourceNode', () => {
        const sourceNode = new MockAPI.WorkletSourceNode(
          context,
          'AudioRuntime',
          () => {}
        );
        expect(sourceNode).toBeInstanceOf(MockAPI.WorkletSourceNode);
      });
    });
  });

  describe('AudioRecorder', () => {
    let recorder: MockAPI.AudioRecorder;

    beforeEach(() => {
      recorder = new MockAPI.AudioRecorder();
    });

    it('should create an AudioRecorder with correct initial state', () => {
      expect(recorder.isRecording()).toBe(false);
      expect(recorder.isPaused()).toBe(false);
      expect(recorder.getCurrentDuration()).toBe(0);
      expect(recorder.options).toBeNull();
    });

    it('should support file output configuration', () => {
      const result = recorder.enableFileOutput({
        format: MockAPI.FileFormat.M4A,
        channelCount: 2,
      });

      expect(result.status).toBe('success');
      expect((result as { path?: string }).path).toBeDefined();
      expect(recorder.options).toBeDefined();
    });

    it('should support recording workflow', () => {
      recorder.enableFileOutput();

      const startResult = recorder.start();
      expect(startResult.status).toBe('success');
      expect(recorder.isRecording()).toBe(true);

      recorder.pause();
      expect(recorder.isPaused()).toBe(true);

      recorder.resume();
      expect(recorder.isPaused()).toBe(false);

      const stopResult = recorder.stop();
      expect(stopResult.status).toBe('success');
      expect((stopResult as { path?: string }).path).toBeDefined();
      expect(recorder.isRecording()).toBe(false);
    });

    it('should support RecorderAdapterNode connection', () => {
      const context = new MockAPI.AudioContext();
      const adapter = context.createRecorderAdapter();

      expect(() => recorder.connect(adapter)).not.toThrow();
      expect(adapter.wasConnected).toBe(true);

      // Should throw on duplicate connection
      expect(() => recorder.connect(adapter)).toThrow();
    });

    it('should support audio data callbacks', () => {
      const callback = jest.fn();
      const result = recorder.onAudioReady(
        { sampleRate: 44100, bufferLength: 1024, channelCount: 2 },
        callback
      );

      expect(result.status).toBe('success');

      recorder.clearOnAudioReady();
      // Should not throw
    });

    it('should support error callbacks', () => {
      const callback = jest.fn();
      expect(() => recorder.onError(callback)).not.toThrow();
      expect(() => recorder.clearOnError()).not.toThrow();
    });
  });

  describe('Utility Functions', () => {
    it('should decode audio data', async () => {
      const arrayBuffer = new ArrayBuffer(1024);
      const buffer = await MockAPI.decodeAudioData(arrayBuffer);

      expect(buffer).toBeInstanceOf(MockAPI.AudioBuffer);
      expect(buffer.numberOfChannels).toBe(2);
      expect(buffer.sampleRate).toBe(44100);
    });

    it('should decode PCM in base64', async () => {
      const base64Data = 'SGVsbG8gV29ybGQ=';
      const buffer = await MockAPI.decodePCMInBase64(base64Data);

      expect(buffer).toBeInstanceOf(MockAPI.AudioBuffer);
    });

    it('should change playback speed', async () => {
      const context = new MockAPI.AudioContext();
      const inputBuffer = context.createBuffer(2, 1024, 44100);
      const outputBuffer = await MockAPI.changePlaybackSpeed(inputBuffer, 1.5);

      expect(outputBuffer).toBe(inputBuffer);
    });
  });

  describe('System Classes', () => {
    describe('AudioManager', () => {
      it('should provide device preferred sample rate', () => {
        const sampleRate = MockAPI.AudioManager.getDevicePreferredSampleRate();
        expect(sampleRate).toBe(44100);
      });

      it('should support volume change observation', () => {
        expect(() =>
          MockAPI.AudioManager.observeVolumeChanges(true)
        ).not.toThrow();
        expect(() =>
          MockAPI.AudioManager.observeVolumeChanges(false)
        ).not.toThrow();
      });

      it('should support system event listeners', () => {
        const callback = jest.fn();
        const listener = MockAPI.AudioManager.addSystemEventListener(
          'volumeChange',
          callback
        );

        expect(listener).toHaveProperty('remove');
        expect(typeof listener.remove).toBe('function');

        expect(() =>
          MockAPI.AudioManager.removeSystemEventListener(listener)
        ).not.toThrow();
      });
    });

    describe('Notification Managers', () => {
      it('should create notification managers', () => {
        const options = { title: 'Test', artist: 'Test Artist' };

        const notification = MockAPI.NotificationManager.create(options);
        expect(notification).toHaveProperty('update');
        expect(notification).toHaveProperty('destroy');

        const playback = MockAPI.PlaybackNotificationManager.create(options);
        expect(playback).toHaveProperty('update');
        expect(playback).toHaveProperty('destroy');

        const recording = MockAPI.RecordingNotificationManager.create(options);
        expect(recording).toHaveProperty('update');
        expect(recording).toHaveProperty('destroy');
      });
    });
  });

  describe('Hooks', () => {
    it('should provide useSystemVolume hook', () => {
      const volume = MockAPI.useSystemVolume();
      expect(typeof volume).toBe('number');
      expect(volume).toBe(0.5);

      MockAPI.setMockSystemVolume(0.8);
      const newVolume = MockAPI.useSystemVolume();
      expect(newVolume).toBe(0.8);
    });
  });

  describe('File Presets', () => {
    it('should provide file preset configurations', () => {
      expect(MockAPI.FilePreset.Low).toBeDefined();
      expect(MockAPI.FilePreset.Medium).toBeDefined();
      expect(MockAPI.FilePreset.High).toBeDefined();
      expect(MockAPI.FilePreset.Lossless).toBeDefined();

      expect(typeof MockAPI.FilePreset.Low).toBe('object');
      expect(typeof MockAPI.FilePreset.Medium).toBe('object');
      expect(typeof MockAPI.FilePreset.High).toBe('object');
      expect(typeof MockAPI.FilePreset.Lossless).toBe('object');
    });
  });

  describe('Error Classes', () => {
    it('should provide custom error classes', () => {
      const notSupportedError = new MockAPI.NotSupportedError('Not supported');
      expect(notSupportedError).toBeInstanceOf(Error);
      expect(notSupportedError.name).toBe('NotSupportedError');

      const invalidAccessError = new MockAPI.InvalidAccessError(
        'Invalid access'
      );
      expect(invalidAccessError).toBeInstanceOf(Error);
      expect(invalidAccessError.name).toBe('InvalidAccessError');

      const invalidStateError = new MockAPI.InvalidStateError('Invalid state');
      expect(invalidStateError).toBeInstanceOf(Error);
      expect(invalidStateError.name).toBe('InvalidStateError');

      const indexSizeError = new MockAPI.IndexSizeError('Index size error');
      expect(indexSizeError).toBeInstanceOf(Error);
      expect(indexSizeError.name).toBe('IndexSizeError');

      const rangeError = new MockAPI.RangeError('Range error');
      expect(rangeError).toBeInstanceOf(Error);
      expect(rangeError.name).toBe('RangeError');

      const audioApiError = new MockAPI.AudioApiError('Audio API error');
      expect(audioApiError).toBeInstanceOf(Error);
      expect(audioApiError.name).toBe('AudioApiError');
    });
  });

  describe('Constants', () => {
    it('should export FileDirectory constants', () => {
      expect(MockAPI.FileDirectory.Document).toBe(0);
      expect(MockAPI.FileDirectory.Cache).toBe(1);
    });

    it('should export FileFormat constants', () => {
      expect(MockAPI.FileFormat.Wav).toBe(0);
      expect(MockAPI.FileFormat.Caf).toBe(1);
    });
  });

  describe('Node Connections', () => {
    let context: MockAPI.AudioContext;

    beforeEach(() => {
      context = new MockAPI.AudioContext();
    });

    it('should support audio node connections', () => {
      const oscillator = context.createOscillator();
      const gainNode = context.createGain();

      const result = oscillator.connect(gainNode);
      expect(result).toBe(gainNode);

      const result2 = gainNode.connect(context.destination);
      expect(result2).toBe(context.destination);
    });

    it('should support AudioParam connections', () => {
      const oscillator = context.createOscillator();
      const gainNode = context.createGain();

      const result = oscillator.connect(gainNode.gain);
      expect(result).toBeUndefined();
    });

    it('should support disconnections', () => {
      const oscillator = context.createOscillator();
      const gainNode = context.createGain();

      oscillator.connect(gainNode);
      expect(() => oscillator.disconnect()).not.toThrow();
      expect(() => oscillator.disconnect(gainNode)).not.toThrow();
    });
  });
});
