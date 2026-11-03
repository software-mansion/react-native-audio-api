import { Platform } from 'react-native';
import { AudioManager } from 'react-native-audio-api';
import { AssistantPlayback } from './audio/AssistantPlayback';
import { VoiceAudioEngine } from './audio/VoiceAudioEngine';
import { encodeCaptureChunk, SAMPLE_RATE } from './audio/pcm';
import { fetchEphemeralKey } from './backend/token';

const CAPTURE_BUFFER_LENGTH = SAMPLE_RATE / 10;

type SessionStatus = 'idle' | 'connecting' | 'connected' | 'error';

export type TranscriptEntry = {
  id: string;
  role: 'user' | 'assistant';
  text: string;
  order: number;
  interrupted?: boolean;
};

type ServerEvent = {
  type: string;
  delta?: string;
  transcript?: string;
  item_id?: string;
  content_index?: number;
  error?: { message?: string };
  response?: {
    status?: string;
    status_details?: { error?: { message?: string } };
  };
};

type SessionHandlers = {
  onStatus: (status: SessionStatus, detail?: string) => void;
  onSpeechActivity: (speaking: boolean) => void;
  onTranscript: (entry: TranscriptEntry) => void;
};

type UserTurn = {
  itemId: string;
  speaking: boolean;
  hasTranscript: boolean;
  responseRequested: boolean;
};

export class VoiceSession {
  private readonly audio = new VoiceAudioEngine();
  private socket: WebSocket | null = null;
  private playback: AssistantPlayback | null = null;
  private active = false;
  private disposed = false;
  private systemInterrupted = false;
  private recording = false;
  private userTurn: UserTurn | null = null;
  private responseInProgress = false;
  private systemTransitionChain: Promise<void> = Promise.resolve();
  private teardownPromise: Promise<void> | null = null;
  private systemSubscriptions: Array<{ remove: () => void }> = [];
  private transcripts = new Map<string, TranscriptEntry>();
  private interruptedTranscriptIds = new Set<string>();
  private connectionAttempt = 0;
  private tokenRequest: AbortController | null = null;
  private audioStartup: Promise<unknown> | null = null;

  constructor(private readonly handlers: SessionHandlers) {}

  async connect(): Promise<void> {
    if (this.disposed) {
      throw new Error('Voice Chat has already been disposed');
    }
    if (this.active) {
      return;
    }

    const attempt = this.invalidateConnectionAttempt();
    await this.teardownPromise;
    if (this.disposed || attempt !== this.connectionAttempt || this.active) {
      return;
    }

    this.active = true;
    this.systemInterrupted = false;
    this.userTurn = null;
    this.responseInProgress = false;
    this.handlers.onStatus('connecting');
    this.transcripts.clear();
    this.interruptedTranscriptIds.clear();

    const tokenRequest = new AbortController();
    this.tokenRequest = tokenRequest;
    try {
      const secret = await fetchEphemeralKey(tokenRequest);
      if (this.tokenRequest === tokenRequest) {
        this.tokenRequest = null;
      }
      if (!this.isCurrentConnectionAttempt(attempt)) {
        return;
      }

      this.subscribeToSystemEvents(attempt);
      const activation = this.audio.activate();
      this.audioStartup = activation;
      const context = await activation;
      if (!this.isCurrentConnectionAttempt(attempt)) {
        return;
      }
      context.onerror = () => this.fail('Audio output failed', attempt);

      this.playback = new AssistantPlayback(
        context,
        () =>
          this.isCurrentConnectionAttempt(attempt) && !this.systemInterrupted,
        (message) => this.fail(message, attempt)
      );

      const socket = new WebSocket(
        `wss://api.openai.com/v1/realtime?model=${encodeURIComponent(secret.model)}`,
        ['realtime', `openai-insecure-api-key.${secret.value}`]
      );
      this.socket = socket;
      const isCurrentSocket = () =>
        this.isCurrentConnectionAttempt(attempt) && this.socket === socket;
      socket.onopen = () => {
        if (!isCurrentSocket()) {
          socket.close();
          return;
        }
        this.handlers.onStatus('connected');
        this.startMicrophone(attempt).catch((error) =>
          this.fail(
            `Could not start the microphone: ${
              error instanceof Error ? error.message : String(error)
            }`,
            attempt
          )
        );
      };
      socket.onmessage = (event) => {
        if (isCurrentSocket()) {
          this.handleEvent(event.data);
        }
      };
      socket.onerror = () => {
        if (isCurrentSocket()) {
          this.fail('WebSocket error. Check the network connection.', attempt);
        }
      };
      socket.onclose = (event) => {
        if (isCurrentSocket()) {
          this.fail(
            event.reason || `Connection closed (${event.code})`,
            attempt
          );
        }
      };
    } catch (error) {
      if (this.isCurrentConnectionAttempt(attempt)) {
        this.fail(
          error instanceof Error ? error.message : String(error),
          attempt
        );
      }
    } finally {
      if (this.tokenRequest === tokenRequest) {
        this.tokenRequest = null;
      }
    }
  }

  async disconnect(): Promise<void> {
    this.invalidateConnectionAttempt();
    if (!this.active) {
      return;
    }
    this.active = false;
    this.handlers.onSpeechActivity(false);
    this.handlers.onStatus('idle');
    await this.teardown();
  }

  async dispose(): Promise<void> {
    this.disposed = true;
    this.invalidateConnectionAttempt();
    if (this.active) {
      await this.disconnect();
    } else {
      await this.teardown();
    }
  }

  private async startMicrophone(
    attempt = this.connectionAttempt
  ): Promise<void> {
    if (
      !this.isCurrentConnectionAttempt(attempt) ||
      this.systemInterrupted ||
      this.recording ||
      this.socket?.readyState !== WebSocket.OPEN
    ) {
      return;
    }

    const recorder = this.audio.getRecorder();
    const registration = recorder.onAudioReady(
      {
        sampleRate: SAMPLE_RATE,
        bufferLength: CAPTURE_BUFFER_LENGTH,
        channelCount: 1,
      },
      ({ buffer, numFrames }) => {
        if (
          !this.isCurrentConnectionAttempt(attempt) ||
          this.systemInterrupted ||
          this.socket?.readyState !== WebSocket.OPEN
        ) {
          return;
        }
        const channel = buffer.getChannelData(0);
        this.send({
          type: 'input_audio_buffer.append',
          audio: encodeCaptureChunk(channel, numFrames, buffer.sampleRate),
        });
      }
    );
    if (registration.status === 'error') {
      throw new Error(registration.message);
    }

    recorder.onError(({ message }) =>
      this.fail(`Recorder error: ${message}`, attempt)
    );
    const startup = recorder.start();
    this.audioStartup = startup;
    const started = await startup;
    if (started.status === 'error') {
      throw new Error(started.message);
    }
    if (!this.isCurrentConnectionAttempt(attempt)) {
      return;
    }
    this.recording = true;
    if (this.systemInterrupted && Platform.OS !== 'ios') {
      await this.audio.stopRecorder(recorder);
      this.recording = false;
    }
  }

  private handleEvent(raw: unknown): void {
    if (typeof raw !== 'string') {
      return;
    }

    let event: ServerEvent;
    try {
      event = JSON.parse(raw) as ServerEvent;
    } catch {
      return;
    }

    switch (event.type) {
      case 'input_audio_buffer.speech_started':
        this.userTurn = event.item_id
          ? {
              itemId: event.item_id,
              speaking: true,
              hasTranscript: false,
              responseRequested: false,
            }
          : null;
        this.handlers.onSpeechActivity(true);
        this.interruptAssistant();
        break;
      case 'input_audio_buffer.speech_stopped':
        this.handlers.onSpeechActivity(false);
        if (this.userTurn && this.userTurn.itemId === event.item_id) {
          this.userTurn.speaking = false;
          this.maybeCreateResponse();
        }
        break;
      case 'response.output_audio.delta':
        if (event.delta) {
          if (
            event.item_id &&
            this.interruptedTranscriptIds.has(event.item_id)
          ) {
            break;
          }
          this.playback?.enqueue(
            event.delta,
            event.item_id,
            event.content_index ?? 0
          );
        }
        break;
      case 'response.output_audio_transcript.delta':
        if (event.delta) {
          this.appendTranscript(event.item_id, 'assistant', event.delta);
        }
        break;
      case 'response.output_audio_transcript.done':
        this.finishTranscript(event.item_id, 'assistant', event.transcript);
        break;
      case 'conversation.item.input_audio_transcription.completed':
        if (event.transcript?.trim()) {
          this.finishTranscript(event.item_id, 'user', event.transcript);
          if (this.userTurn && this.userTurn.itemId === event.item_id) {
            this.userTurn.hasTranscript = true;
            this.maybeCreateResponse();
          }
        } else if (event.item_id) {
          this.send({
            type: 'conversation.item.delete',
            item_id: event.item_id,
          });
        }
        break;
      case 'conversation.item.input_audio_transcription.failed':
        this.fail(
          event.error?.message ?? 'Could not transcribe microphone audio'
        );
        break;
      case 'response.created':
        this.responseInProgress = true;
        break;
      case 'response.done':
        this.responseInProgress = false;
        this.playback?.finishResponse();
        if (event.response?.status === 'failed') {
          this.fail(
            event.response.status_details?.error?.message ??
              'Realtime response failed'
          );
        } else {
          this.maybeCreateResponse();
        }
        break;
      case 'error':
        this.fail(event.error?.message ?? 'Realtime API error');
        break;
      default:
        break;
    }
  }

  private maybeCreateResponse(): void {
    const turn = this.userTurn;
    if (
      !this.active ||
      this.systemInterrupted ||
      !turn ||
      turn.speaking ||
      !turn.hasTranscript ||
      turn.responseRequested ||
      this.responseInProgress ||
      this.socket?.readyState !== WebSocket.OPEN
    ) {
      return;
    }
    // Reserve the response before response.created arrives from the server.
    turn.responseRequested = true;
    this.responseInProgress = true;
    this.send({ type: 'response.create' });
  }

  private appendTranscript(
    itemId: string | undefined,
    role: TranscriptEntry['role'],
    delta: string
  ): void {
    const id = itemId ?? `${role}-pending`;
    this.updateTranscript(
      id,
      role,
      `${this.transcripts.get(id)?.text ?? ''}${delta}`
    );
  }

  private finishTranscript(
    itemId: string | undefined,
    role: TranscriptEntry['role'],
    text: string | undefined
  ): void {
    const id = itemId ?? `${role}-${Date.now()}`;
    const finalText = text ?? this.transcripts.get(id)?.text;
    if (!finalText) {
      return;
    }
    this.updateTranscript(id, role, finalText);
  }

  private updateTranscript(
    id: string,
    role: TranscriptEntry['role'],
    text: string
  ): void {
    const entry: TranscriptEntry = {
      id,
      role,
      text,
      order: this.transcripts.get(id)?.order ?? this.transcripts.size,
      interrupted:
        role === 'assistant' && this.interruptedTranscriptIds.has(id),
    };
    this.transcripts.set(id, entry);
    this.handlers.onTranscript(entry);
  }

  private send(event: object): void {
    if (this.socket?.readyState !== WebSocket.OPEN) {
      return;
    }
    this.socket.send(JSON.stringify(event));
  }

  private fail(message: string, attempt = this.connectionAttempt): void {
    if (!this.isCurrentConnectionAttempt(attempt)) {
      return;
    }
    this.active = false;
    this.invalidateConnectionAttempt();
    this.handlers.onSpeechActivity(false);
    this.handlers.onStatus('error', message);
    this.teardown().catch(() => undefined);
  }

  private async teardown(): Promise<void> {
    if (this.teardownPromise) {
      return this.teardownPromise;
    }

    this.teardownPromise = this.performTeardown();
    try {
      await this.teardownPromise;
    } finally {
      this.teardownPromise = null;
    }
  }

  private async performTeardown(): Promise<void> {
    this.systemInterrupted = false;
    this.unsubscribeFromSystemEvents();
    // Native startup cannot be aborted; finish it before releasing its resources.
    await this.audioStartup?.catch(() => undefined);
    this.audioStartup = null;
    await this.systemTransitionChain.catch(() => undefined);
    this.recording = false;
    await this.audio.stopRecorder().catch(() => undefined);

    try {
      this.socket?.close();
    } catch {
      // The native socket may already be closed after a connection failure.
    }
    this.socket = null;

    this.playback?.close();
    this.playback = null;
    await this.audio.deactivate().catch(() => undefined);
    await this.audio.close().catch(() => undefined);
  }

  private interruptAssistant(): void {
    const audio = this.playback?.interrupt();
    if (
      this.active &&
      audio &&
      !this.interruptedTranscriptIds.has(audio.itemId)
    ) {
      this.send({
        type: 'conversation.item.truncate',
        item_id: audio.itemId,
        content_index: audio.contentIndex,
        audio_end_ms: Math.floor(audio.renderedMs),
      });
      this.interruptedTranscriptIds.add(audio.itemId);
      const entry = this.transcripts.get(audio.itemId);
      if (entry) {
        this.updateTranscript(audio.itemId, 'assistant', entry.text);
      }
    }
  }

  private subscribeToSystemEvents(attempt: number): void {
    if (this.systemSubscriptions.length > 0) {
      return;
    }
    this.systemSubscriptions = [
      AudioManager.addSystemEventListener('interruption', (event) => {
        if (!this.isCurrentConnectionAttempt(attempt)) {
          return;
        }
        if (event.type === 'began') {
          this.pauseForSystemEvent();
        } else if (event.shouldResume) {
          this.resumeAfterSystemEvent();
        } else {
          this.fail('Audio focus was not restored after an interruption');
        }
      }),
      AudioManager.addSystemEventListener('duck', () => {
        if (this.isCurrentConnectionAttempt(attempt)) {
          this.pauseForSystemEvent();
        }
      }),
    ];
  }

  private unsubscribeFromSystemEvents(): void {
    for (const subscription of this.systemSubscriptions) {
      subscription.remove();
    }
    this.systemSubscriptions = [];
  }

  private pauseForSystemEvent(): void {
    if (!this.active || this.systemInterrupted) {
      return;
    }
    this.systemInterrupted = true;
    this.userTurn = null;
    this.handlers.onSpeechActivity(false);
    this.interruptAssistant();
    this.runSystemTransition(async () => {
      if (!this.active) {
        return;
      }
      // iOS keeps the recorder registered and resumes it natively. Stopping it
      // here can prevent interruption-ended delivery; callbacks are gated above.
      if (Platform.OS !== 'ios') {
        this.recording = false;
        await this.audio.stopRecorder();
      }
      const context = this.audio.getContext();
      if (context.state === 'running') {
        await context.suspend();
      }
    });
  }

  private resumeAfterSystemEvent(): void {
    if (!this.active || !this.systemInterrupted) {
      return;
    }
    this.runSystemTransition(async () => {
      if (!this.active || !this.systemInterrupted) {
        return;
      }
      this.systemInterrupted = false;
      const context = this.audio.getContext();
      if (context.state === 'suspended') {
        await context.resume();
      }
      await this.startMicrophone();
    });
  }

  private runSystemTransition(operation: () => Promise<void>): void {
    const attempt = this.connectionAttempt;
    this.systemTransitionChain = this.systemTransitionChain
      .then(() => {
        if (this.isCurrentConnectionAttempt(attempt)) {
          return operation();
        }
      })
      .catch((error) =>
        this.fail(
          `Could not recover from a system audio interruption: ${String(error)}`,
          attempt
        )
      );
  }

  private invalidateConnectionAttempt(): number {
    this.tokenRequest?.abort();
    this.tokenRequest = null;
    this.connectionAttempt += 1;
    return this.connectionAttempt;
  }

  private isCurrentConnectionAttempt(attempt: number): boolean {
    return !this.disposed && this.active && attempt === this.connectionAttempt;
  }
}

export type { SessionStatus };
