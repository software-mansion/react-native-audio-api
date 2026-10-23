import { VoiceSession } from './voiceSession';

const mockQueue = {
  clearBuffers: jest.fn(),
  connect: jest.fn(),
  disconnect: jest.fn(),
  enqueueBuffer: jest.fn(() => 'buffer-1'),
  onbufferended: null as
    | ((event: { bufferId: string; isLastBufferInQueue: boolean }) => void)
    | null,
  start: jest.fn(),
  stop: jest.fn(),
};

const mockContext = {
  close: jest.fn(),
  createBufferQueueSource: jest.fn(() => mockQueue),
  currentTime: 0,
  decodePCMInBase64: jest.fn(async () => ({ duration: 0.1 })),
  destination: {},
  resume: jest.fn(),
  state: 'running',
  suspend: jest.fn(),
};

jest.mock('react-native', () => ({
  NativeModules: { SourceCode: { getConstants: () => ({ scriptURL: '' }) } },
  Platform: { OS: 'android', Version: 31 },
}));

jest.mock('react-native-audio-api', () => {
  const recorder = {
    clearOnAudioReady: jest.fn(),
    clearOnError: jest.fn(),
    isRecording: jest.fn(() => false),
    onAudioReady: jest.fn(() => ({ status: 'success' })),
    onError: jest.fn(),
    start: jest.fn(async () => ({ status: 'success' })),
    stop: jest.fn(async () => undefined),
  };
  return {
    AudioContext: jest.fn(() => mockContext),
    AudioManager: {
      addSystemEventListener: jest.fn(() => ({ remove: jest.fn() })),
      getCommunicationDevice: jest.fn(async () => ({
        category: 'Built-in Speaker',
      })),
      observeAudioInterruptions: jest.fn(),
      checkRecordingPermissions: jest.fn(async () => 'Granted'),
      requestRecordingPermissions: jest.fn(async () => 'Granted'),
      setAudioSessionActivity: jest.fn(async () => undefined),
      setSystemOptions: jest.fn(),
    },
    AudioRecorder: jest.fn(() => recorder),
    __mockRecorder: recorder,
  };
});

const mockAudioApi = jest.requireMock('react-native-audio-api');
const mockAudioManager = mockAudioApi.AudioManager;
const mockRecorder = mockAudioApi.__mockRecorder;
const mockPlatform = jest.requireMock('react-native').Platform;

class MockWebSocket {
  static readonly OPEN = 1;
  static readonly instances: MockWebSocket[] = [];

  onclose: ((event: { code: number; reason: string }) => void) | null = null;
  onerror: (() => void) | null = null;
  onmessage: ((event: { data: string }) => void) | null = null;
  onopen: (() => void) | null = null;
  readyState = 0;
  sent: string[] = [];

  constructor() {
    MockWebSocket.instances.push(this);
  }

  close(): void {
    this.readyState = 3;
  }

  open(): void {
    this.readyState = MockWebSocket.OPEN;
    this.onopen?.();
  }

  send(data: string): void {
    this.sent.push(data);
  }
}

function emit(session: VoiceSession, event: object): void {
  (session as unknown as { handleEvent: (raw: string) => void }).handleEvent(
    JSON.stringify(event)
  );
}

function createSession() {
  const onStatus = jest.fn();
  const onTranscript = jest.fn();
  const session = new VoiceSession({
    onSpeechActivity: jest.fn(),
    onStatus,
    onTranscript,
  });
  return { session, onStatus, onTranscript };
}

async function flush(): Promise<void> {
  await Promise.resolve();
  await Promise.resolve();
}

describe('VoiceSession interruptions', () => {
  beforeEach(() => {
    jest.clearAllMocks();
    mockPlatform.OS = 'android';
    MockWebSocket.instances.length = 0;
    mockQueue.onbufferended = null;
    mockContext.decodePCMInBase64.mockResolvedValue({ duration: 0.1 });
    mockContext.close.mockResolvedValue(undefined);
    mockContext.resume.mockImplementation(async () => {
      mockContext.state = 'running';
    });
    mockContext.suspend.mockImplementation(async () => {
      mockContext.state = 'suspended';
    });
    mockRecorder.isRecording.mockReturnValue(false);
    mockRecorder.stop.mockImplementation(async () => {
      mockRecorder.isRecording.mockReturnValue(false);
    });
    mockRecorder.start.mockResolvedValue({ status: 'success' });
    mockAudioManager.getCommunicationDevice.mockResolvedValue({
      category: 'Built-in Speaker',
    });
    mockAudioManager.checkRecordingPermissions.mockResolvedValue('Granted');
    globalThis.WebSocket = MockWebSocket as unknown as typeof WebSocket;
    globalThis.fetch = jest.fn(async () => ({
      json: async () => ({ model: 'gpt-realtime-2.1-mini', value: 'secret' }),
      ok: true,
    })) as unknown as typeof fetch;
  });

  async function connect(): Promise<{
    session: VoiceSession;
    socket: MockWebSocket;
    onStatus: jest.Mock;
  }> {
    const { session, onStatus } = createSession();
    await session.connect();
    const socket = MockWebSocket.instances[0];
    socket.open();
    await flush();
    return { session, socket, onStatus };
  }

  it('does not answer a superseded turn while the user is speaking', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'input_audio_buffer.speech_stopped',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'user-2',
    });
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'user-1',
      transcript: 'The earlier question',
    });
    expect(socket.sent).toHaveLength(0);
    emit(session, {
      type: 'input_audio_buffer.speech_stopped',
      item_id: 'user-2',
    });
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'user-2',
      transcript: 'Actually, another question',
    });
    expect(socket.sent.map((event) => JSON.parse(event))).toEqual([
      { type: 'response.create' },
    ]);
    await session.dispose();
  });

  it('waits for speech to stop even if transcription completes first', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'user-1',
      transcript: 'Hello',
    });
    expect(socket.sent).toHaveLength(0);
    emit(session, {
      type: 'input_audio_buffer.speech_stopped',
      item_id: 'user-1',
    });
    expect(socket.sent.map((event) => JSON.parse(event))).toEqual([
      { type: 'response.create' },
    ]);
    await session.dispose();
  });

  it('waits for an outstanding response before answering the latest turn', async () => {
    const { session, socket } = await connect();
    for (const id of ['user-1', 'user-2']) {
      emit(session, { type: 'input_audio_buffer.speech_started', item_id: id });
      emit(session, { type: 'input_audio_buffer.speech_stopped', item_id: id });
      emit(session, {
        type: 'conversation.item.input_audio_transcription.completed',
        item_id: id,
        transcript: 'Hello',
      });
    }
    expect(
      socket.sent.filter((event) => event.includes('response.create'))
    ).toHaveLength(1);
    emit(session, { type: 'response.done', response: { status: 'cancelled' } });
    expect(
      socket.sent.filter((event) => event.includes('response.create'))
    ).toHaveLength(2);
    await session.dispose();
  });

  it('discards a pending reply if a newer speech turn starts', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'response.created',
      response: { status: 'in_progress' },
    });
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'input_audio_buffer.speech_stopped',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'user-1',
      transcript: 'Hello',
    });
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'user-2',
    });
    emit(session, { type: 'response.done', response: { status: 'cancelled' } });
    expect(socket.sent).toHaveLength(0);
    await session.dispose();
  });

  it('retains the iOS recorder and callbacks across native interruption recovery', async () => {
    mockPlatform.OS = 'ios';
    let recording = false;
    let interrupted = false;
    mockRecorder.isRecording.mockImplementation(
      () => recording && !interrupted
    );
    mockRecorder.start.mockImplementation(async () => {
      if (recording) {
        return { status: 'error', message: 'Recorder is already recording' };
      }
      recording = true;
      return { status: 'success' };
    });
    const { session, socket, onStatus } = await connect();
    const listeners = new Map<string, (event: object) => void>(
      mockAudioManager.addSystemEventListener.mock.calls
    );
    const capture = mockRecorder.onAudioReady.mock.calls[0][1];
    const chunk = {
      buffer: {
        getChannelData: () => new Float32Array([0]),
        sampleRate: 24000,
      },
      numFrames: 1,
    };
    interrupted = true;
    listeners.get('interruption')?.({ type: 'began', shouldResume: false });
    await flush();
    capture(chunk);
    expect(socket.sent).toHaveLength(0);
    expect(mockRecorder.clearOnAudioReady).not.toHaveBeenCalled();
    expect(mockRecorder.stop).not.toHaveBeenCalled();
    interrupted = false;
    listeners.get('interruption')?.({ type: 'ended', shouldResume: true });
    await flush();
    await flush();
    expect(mockRecorder.start).toHaveBeenCalledTimes(1);
    expect(onStatus).toHaveBeenLastCalledWith('connected');
    capture(chunk);
    expect(socket.sent.map((event) => JSON.parse(event))).toContainEqual({
      type: 'input_audio_buffer.append',
      audio: 'AAA=',
    });
    await session.dispose();
  });

  it('retains an iOS recorder whose startup completes during an interruption', async () => {
    mockPlatform.OS = 'ios';
    let finishStarting: ((value: { status: 'success' }) => void) | undefined;
    mockRecorder.start.mockImplementation(
      () =>
        new Promise((resolve) => {
          finishStarting = resolve;
        })
    );
    const { session, onStatus } = await connect();
    const listeners = new Map<string, (event: object) => void>(
      mockAudioManager.addSystemEventListener.mock.calls
    );
    listeners.get('interruption')?.({ type: 'began', shouldResume: false });
    await flush();
    finishStarting?.({ status: 'success' });
    await flush();
    listeners.get('interruption')?.({ type: 'ended', shouldResume: true });
    await flush();
    await flush();
    expect(mockRecorder.start).toHaveBeenCalledTimes(1);
    expect(mockRecorder.stop).not.toHaveBeenCalled();
    expect(mockRecorder.clearOnAudioReady).not.toHaveBeenCalled();
    expect(onStatus).toHaveBeenLastCalledWith('connected');
    await session.dispose();
  });

  it('does not create a reply for an empty transcription', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'noise',
    });
    emit(session, {
      type: 'input_audio_buffer.speech_stopped',
      item_id: 'noise',
    });
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'noise',
      transcript: '   ',
    });
    emit(session, { type: 'response.done', response: { status: 'cancelled' } });
    expect(socket.sent.map((event) => JSON.parse(event))).toEqual([
      { type: 'conversation.item.delete', item_id: 'noise' },
    ]);
    await session.dispose();
  });

  it('does not create a pending reply after disconnect', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'response.created',
      response: { status: 'in_progress' },
    });
    emit(session, {
      type: 'input_audio_buffer.speech_started',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'input_audio_buffer.speech_stopped',
      item_id: 'user-1',
    });
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'user-1',
      transcript: 'Hello',
    });
    await session.disconnect();
    socket.onmessage?.({
      data: JSON.stringify({
        type: 'response.done',
        response: { status: 'cancelled' },
      }),
    });
    expect(socket.sent).toHaveLength(0);
    await session.dispose();
  });

  it('checks permission without opening a permission activity during startup', async () => {
    const { session } = await connect();
    expect(mockAudioManager.checkRecordingPermissions).toHaveBeenCalledTimes(1);
    expect(mockAudioManager.requestRecordingPermissions).not.toHaveBeenCalled();
    await session.dispose();
  });

  it('does not activate audio when microphone permission is denied', async () => {
    mockAudioManager.checkRecordingPermissions.mockResolvedValue('Denied');
    const { session, onStatus } = createSession();
    await session.connect();
    expect(onStatus).toHaveBeenCalledWith(
      'error',
      'Microphone permission was not granted'
    );
    expect(mockAudioManager.setAudioSessionActivity).not.toHaveBeenCalledWith(
      true
    );
    expect(MockWebSocket.instances).toHaveLength(0);
    await session.dispose();
  });

  it('does not play PCM decoded after an interruption', async () => {
    let resolveDecode: ((value: { duration: number }) => void) | undefined;
    mockContext.decodePCMInBase64.mockImplementation(
      () =>
        new Promise((resolve) => {
          resolveDecode = resolve;
        })
    );
    const { session, socket } = await connect();

    emit(session, {
      type: 'response.output_audio.delta',
      item_id: 'assistant-1',
      content_index: 0,
      delta: 'pcm',
    });
    await flush();
    expect(resolveDecode).toBeDefined();
    emit(session, { type: 'response.done', response: { status: 'completed' } });
    emit(session, { type: 'input_audio_buffer.speech_started' });
    resolveDecode?.({ duration: 0.1 });
    await flush();

    expect(mockQueue.enqueueBuffer).not.toHaveBeenCalled();
    expect(socket.sent.map((event) => JSON.parse(event))).toContainEqual({
      type: 'conversation.item.truncate',
      item_id: 'assistant-1',
      content_index: 0,
      audio_end_ms: 0,
    });
  });

  it('truncates one active response at the completed-buffer boundary', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'response.output_audio.delta',
      item_id: 'assistant-1',
      content_index: 0,
      delta: 'pcm',
    });
    await flush();
    mockQueue.onbufferended?.({
      bufferId: 'buffer-1',
      isLastBufferInQueue: false,
    });

    emit(session, { type: 'input_audio_buffer.speech_started' });
    emit(session, { type: 'input_audio_buffer.speech_started' });

    expect(socket.sent.map((event) => JSON.parse(event))).toContainEqual({
      type: 'conversation.item.truncate',
      item_id: 'assistant-1',
      content_index: 0,
      audio_end_ms: 100,
    });
    expect(
      socket.sent.filter((event) => event.includes('item.truncate'))
    ).toHaveLength(1);
  });

  it('starts the playback queue once across multiple audio chunks', async () => {
    const { session } = await connect();
    const event = {
      type: 'response.output_audio.delta',
      item_id: 'assistant-1',
      delta: 'pcm',
    };
    emit(session, event);
    await flush();
    emit(session, event);
    await flush();
    await flush();
    expect(mockQueue.enqueueBuffer).toHaveBeenCalledTimes(2);
    expect(mockQueue.start).toHaveBeenCalledTimes(1);
    await session.dispose();
  });

  it('ignores late audio chunks from an interrupted assistant item', async () => {
    const { session } = await connect();
    const event = {
      type: 'response.output_audio.delta',
      item_id: 'assistant-1',
      delta: 'pcm',
    };
    emit(session, event);
    await flush();
    emit(session, { type: 'input_audio_buffer.speech_started' });
    emit(session, event);
    await flush();
    expect(mockQueue.enqueueBuffer).toHaveBeenCalledTimes(1);
    await session.dispose();
  });

  it('retains transcript order when replacing streamed text with final text', async () => {
    const { session, onTranscript } = createSession();
    await session.connect();
    emit(session, {
      type: 'conversation.item.input_audio_transcription.completed',
      item_id: 'user-1',
      transcript: 'Hello',
    });
    emit(session, {
      type: 'response.output_audio_transcript.delta',
      item_id: 'assistant-1',
      delta: 'Hi',
    });
    emit(session, {
      type: 'response.output_audio_transcript.done',
      item_id: 'assistant-1',
      transcript: 'Hi there',
    });
    expect(
      onTranscript.mock.calls.map(([entry]) => [entry.text, entry.order])
    ).toEqual([
      ['Hello', 0],
      ['Hi', 1],
      ['Hi there', 1],
    ]);
    await session.dispose();
  });

  it('does not truncate audio that has finished normally', async () => {
    const { session, socket } = await connect();
    emit(session, {
      type: 'response.output_audio.delta',
      item_id: 'assistant-1',
      content_index: 0,
      delta: 'pcm',
    });
    await flush();
    mockQueue.onbufferended?.({
      bufferId: 'buffer-1',
      isLastBufferInQueue: true,
    });
    emit(session, { type: 'response.done', response: { status: 'completed' } });

    emit(session, { type: 'input_audio_buffer.speech_started' });

    expect(
      socket.sent.filter((event) => event.includes('item.truncate'))
    ).toHaveLength(0);
  });

  it('marks an interrupted assistant transcript', async () => {
    const { session, onTranscript } = createSession();
    await session.connect();
    MockWebSocket.instances[0].open();
    await flush();
    emit(session, {
      type: 'response.output_audio_transcript.delta',
      item_id: 'assistant-1',
      delta: 'Hello',
    });
    emit(session, {
      type: 'response.output_audio.delta',
      item_id: 'assistant-1',
      content_index: 0,
      delta: 'pcm',
    });
    await flush();

    emit(session, { type: 'input_audio_buffer.speech_started' });

    expect(onTranscript).toHaveBeenLastCalledWith(
      expect.objectContaining({ id: 'assistant-1', interrupted: true })
    );
  });

  it('closes audio activated after disconnect', async () => {
    let resolveRoute:
      | ((value: { category: 'Built-in Speaker' }) => void)
      | undefined;
    let notifyRouteRequested: (() => void) | undefined;
    const routeRequested = new Promise<void>((resolve) => {
      notifyRouteRequested = resolve;
    });
    mockAudioManager.getCommunicationDevice.mockImplementation(() => {
      notifyRouteRequested?.();
      return new Promise((resolve) => {
        resolveRoute = resolve;
      });
    });
    const { session } = createSession();

    const connecting = session.connect();
    await routeRequested;
    const disconnecting = session.disconnect();
    resolveRoute?.({ category: 'Built-in Speaker' });
    await connecting;
    await disconnecting;

    expect(mockContext.close).toHaveBeenCalledTimes(1);
  });

  it('stops a recorder that finishes starting after disconnect', async () => {
    let resolveStart: ((value: { status: 'success' }) => void) | undefined;
    mockRecorder.start.mockImplementation(
      () =>
        new Promise((resolve) => {
          resolveStart = resolve;
        })
    );
    mockRecorder.isRecording.mockReturnValue(true);
    const { session } = await connect();

    const disconnecting = session.disconnect();
    resolveStart?.({ status: 'success' });
    await disconnecting;
    await flush();

    expect(mockRecorder.stop).toHaveBeenCalledTimes(1);
  });

  it('ignores a stale token response after reconnecting', async () => {
    const resolveRequests: Array<(response: unknown) => void> = [];
    globalThis.fetch = jest.fn(
      () =>
        new Promise((resolve) => {
          resolveRequests.push(resolve);
        })
    ) as unknown as typeof fetch;
    const { session } = createSession();
    const tokenResponse = {
      json: async () => ({ model: 'gpt-realtime-2.1-mini', value: 'secret' }),
      ok: true,
    };

    const firstConnection = session.connect();
    await flush();
    await session.disconnect();
    expect(
      (globalThis.fetch as jest.Mock).mock.calls[0][1].signal.aborted
    ).toBe(true);

    const secondConnection = session.connect();
    await flush();
    resolveRequests[1](tokenResponse);
    await secondConnection;
    resolveRequests[0](tokenResponse);
    await firstConnection;

    expect(MockWebSocket.instances).toHaveLength(1);
  });

  it('finishes cancelled native activation before starting the next call', async () => {
    let resolveRoute: ((value: { category: string }) => void) | undefined;
    const routeRequested = new Promise<void>((requested) => {
      mockAudioManager.getCommunicationDevice.mockImplementationOnce(
        () =>
          new Promise((resolve) => {
            resolveRoute = resolve;
            requested();
          })
      );
    });
    const { session } = createSession();
    const firstConnection = session.connect();
    await routeRequested;
    const disconnecting = session.disconnect();
    const secondConnection = session.connect();
    await flush();
    expect(globalThis.fetch).toHaveBeenCalledTimes(1);

    resolveRoute?.({ category: 'Built-in Speaker' });
    await Promise.all([firstConnection, disconnecting, secondConnection]);

    expect(MockWebSocket.instances).toHaveLength(1);
    expect(mockContext.close).toHaveBeenCalledTimes(1);
    expect(globalThis.fetch).toHaveBeenCalledTimes(2);
  });

  it('recovers capture after a resumable communication-focus duck', async () => {
    mockRecorder.start.mockImplementation(async () => {
      mockRecorder.isRecording.mockReturnValue(true);
      return { status: 'success' };
    });
    const { session } = await connect();
    const listeners = new Map<
      string,
      (event: { type?: string; shouldResume?: boolean }) => void
    >(mockAudioManager.addSystemEventListener.mock.calls);
    listeners.get('duck')?.({});
    await flush();
    expect(mockRecorder.stop).toHaveBeenCalledTimes(1);
    listeners.get('interruption')?.({ type: 'ended', shouldResume: true });
    await flush();
    await flush();
    expect(mockRecorder.start).toHaveBeenCalledTimes(2);
    await session.disconnect();
  });
});
