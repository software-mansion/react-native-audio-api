import { InvalidStateError, NotSupportedError } from '../errors';
import { AudioEventEmitter } from '../events';
import { OnStateChangeEventType } from '../events/types';
import { IBaseAudioContext } from '../jsi-interfaces';
import {
  ContextState,
  DecodeDataInput,
  AudioBufferQueueSourceOptions,
} from '../types';
import AnalyserNode from './AnalyserNode';
import AudioBuffer from './AudioBuffer';
import AudioBufferQueueSourceNode from './AudioBufferQueueSourceNode';
import AudioBufferSourceNode from './AudioBufferSourceNode';
import { decodeAudioData, decodePCMInBase64 } from './AudioDecoder';
import AudioDestinationNode from './AudioDestinationNode';
import AudioListener from './AudioListener';
import BiquadFilterNode from './BiquadFilterNode';
import ChannelMergerNode from './ChannelMergerNode';
import ChannelSplitterNode from './ChannelSplitterNode';
import ConstantSourceNode from './ConstantSourceNode';
import ConvolverNode from './ConvolverNode';
import DelayNode from './DelayNode';
import GainNode from './GainNode';
import IIRFilterNode from './IIRFilterNode';
import OscillatorNode from './OscillatorNode';
import PeriodicWave from './PeriodicWave';
import StereoPannerNode from './StereoPannerNode';
import WaveShaperNode from './WaveShaperNode';

export interface ContextStateChangeEvent {
  type: 'statechange';
  target: BaseAudioContext;
}

export default class BaseAudioContext {
  readonly destination: AudioDestinationNode;
  readonly listener: AudioListener;
  readonly sampleRate: number;
  readonly context: IBaseAudioContext;

  constructor(context: IBaseAudioContext) {
    this.context = context;
    this.destination = new AudioDestinationNode(this, context.destination);
    this.listener = new AudioListener(this, context.listener);
    this.sampleRate = context.sampleRate;

    // The native context owns statechange: it dispatches once per acknowledged
    // transition, after settling the operation's promise, so the event always
    // lands in a later task than the promise continuations. This subscription
    // lives as long as the context; native transitions that no JS call
    // requested (e.g. a future interrupted state) flow through the same path.
    this.stateChangeSubscription = this.audioEventEmitter.addAudioEventListener(
      'stateChange',
      (event: OnStateChangeEventType) => this.onNativeStateChange(event)
    );
    this.context.onstatechange = this.stateChangeSubscription.subscriptionId;
  }

  /**
   * The spec's [[control thread state]]: written synchronously the moment an
   * operation is accepted, so the NEXT call validates against what has already
   * been requested (e.g. close() right after resume() must see 'running').
   * Never exposed — the `state` attribute reports acknowledged reality
   * instead.
   */
  protected _state: ContextState = 'suspended';

  protected readonly audioEventEmitter = new AudioEventEmitter(
    globalThis.AudioEventEmitter
  );

  private stateChangeSubscription: ReturnType<
    AudioEventEmitter['addAudioEventListener']
  >;

  private onstatechangeCallback:
    | ((event: ContextStateChangeEvent) => void)
    | null = null;

  /**
   * Web Audio API `statechange` event handler. Dispatched by the native context
   * from a queued task after the `state` attribute changes — after the
   * operation's promise resolution and all of its microtasks, matching the
   * spec's media-element-task order.
   */
  public get onstatechange():
    | ((event: ContextStateChangeEvent) => void)
    | null {
    return this.onstatechangeCallback;
  }

  public set onstatechange(
    callback: ((event: ContextStateChangeEvent) => void) | null
  ) {
    this.onstatechangeCallback = callback;
  }

  /**
   * Terminal handler for the native statechange dispatch. Deliberately does NOT
   * write `state`: the attribute is published in the resolution task of the
   * operation that caused the transition (spec order), and a rapid
   * resume()+suspend() pair acknowledges both before either event lands — a
   * write here would roll the attribute back to the stale event's value. When a
   * native-originated state with no acknowledging promise arrives (the planned
   * `interrupted`), its attribute update must be added here explicitly.
   * Subclasses extend this to order dependent events (offline `complete`) after
   * `statechange`.
   */
  protected onNativeStateChange(_event: OnStateChangeEventType): void {
    this.onstatechangeCallback?.({ type: 'statechange', target: this });
  }

  /**
   * Record that a state transition has been requested ([[control thread
   * state]]).
   */
  protected setControlState(nextState: ContextState): void {
    this._state = nextState;
  }

  public get currentTime(): number {
    return this.context.currentTime;
  }

  public get state(): ContextState {
    return this.context.state as ContextState;
  }

  /**
   * @internal Called by AudioScheduledSourceNode.start(). No-op here: only
   * AudioContext overrides it, since only AudioContext's native driver can
   * start implicitly from a source's start() call. OfflineAudioContext only
   * starts rendering from an explicit startRendering() call.
   */
  public markRunningOnSourceStart(): void {}

  public async decodeAudioData(
    input: DecodeDataInput,
    fetchOptions?: RequestInit
  ): Promise<AudioBuffer> {
    return await decodeAudioData(input, this.sampleRate, fetchOptions);
  }

  public async decodePCMInBase64(
    base64String: string,
    inputSampleRate: number,
    inputChannelCount: number,
    isInterleaved: boolean = true
  ): Promise<AudioBuffer> {
    return await decodePCMInBase64(
      base64String,
      inputSampleRate,
      inputChannelCount,
      isInterleaved
    );
  }

  createOscillator(): OscillatorNode {
    return new OscillatorNode(this);
  }

  createConstantSource(): ConstantSourceNode {
    return new ConstantSourceNode(this);
  }

  createGain(): GainNode {
    return new GainNode(this);
  }

  createDelay(maxDelayTime?: number): DelayNode {
    if (maxDelayTime !== undefined) {
      return new DelayNode(this, { maxDelayTime });
    } else {
      return new DelayNode(this);
    }
  }

  createStereoPanner(): StereoPannerNode {
    return new StereoPannerNode(this);
  }

  createBiquadFilter(): BiquadFilterNode {
    return new BiquadFilterNode(this);
  }

  createBufferSource(options?: {
    pitchCorrection: boolean;
  }): AudioBufferSourceNode {
    if (options !== undefined) {
      return new AudioBufferSourceNode(this, options);
    } else {
      return new AudioBufferSourceNode(this);
    }
  }

  createIIRFilter(feedforward: number[], feedback: number[]): IIRFilterNode {
    if (feedforward.length < 1 || feedforward.length > 20) {
      throw new NotSupportedError(
        `The provided feedforward array has length (${feedforward.length}) outside the range [1, 20]`
      );
    }
    if (feedback.length < 1 || feedback.length > 20) {
      throw new NotSupportedError(
        `The provided feedback array has length (${feedback.length}) outside the range [1, 20]`
      );
    }

    if (feedforward.every((value) => value === 0)) {
      throw new InvalidStateError(
        `Feedforward array must contain at least one non-zero value`
      );
    }

    if (feedback[0] === 0) {
      throw new InvalidStateError(
        `First value of feedback array cannot be zero`
      );
    }

    return new IIRFilterNode(this, { feedforward, feedback });
  }

  createBufferQueueSource(
    options?: AudioBufferQueueSourceOptions
  ): AudioBufferQueueSourceNode {
    if (options !== undefined) {
      return new AudioBufferQueueSourceNode(this, options);
    } else {
      return new AudioBufferQueueSourceNode(this);
    }
  }

  createBuffer(
    numberOfChannels: number,
    length: number,
    sampleRate: number
  ): AudioBuffer {
    return new AudioBuffer({ numberOfChannels, length, sampleRate });
  }

  createPeriodicWave(
    real: Float32Array,
    imag: Float32Array,
    constraints?: PeriodicWaveConstraints
  ): PeriodicWave {
    return new PeriodicWave(this, { real, imag, ...constraints });
  }

  createAnalyser(): AnalyserNode {
    return new AnalyserNode(this);
  }

  createConvolver(): ConvolverNode {
    return new ConvolverNode(this);
  }

  createWaveShaper(): WaveShaperNode {
    return new WaveShaperNode(this);
  }

  createChannelMerger(numberOfInputs?: number): ChannelMergerNode {
    return new ChannelMergerNode(
      this,
      numberOfInputs !== undefined ? { numberOfInputs } : undefined
    );
  }

  createChannelSplitter(numberOfOutputs?: number): ChannelSplitterNode {
    return new ChannelSplitterNode(
      this,
      numberOfOutputs !== undefined ? { numberOfOutputs } : undefined
    );
  }
}
