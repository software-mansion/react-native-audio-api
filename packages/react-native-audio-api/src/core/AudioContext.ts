import { InvalidStateError } from '../errors';
import { assertSupportedSampleRate } from '../utils/validation';
import { AudioTagHandle } from '../Audio/types';
import { AudioEventEmitter } from '../events';
import { IAudioContext } from '../jsi-interfaces';
import AudioManager from '../system';
import { AudioContextOptions, ContextState } from '../types';
import BaseAudioContext from './BaseAudioContext';
import MediaElementAudioSourceNode from './MediaElementAudioSourceNode';

export default class AudioContext extends BaseAudioContext {
  public onerror: (() => void) | null = null;

  private readonly errorSubscription: ReturnType<
    AudioEventEmitter['addAudioEventListener']
  >;

  constructor(options?: AudioContextOptions) {
    if (options?.sampleRate != null) {
      assertSupportedSampleRate(options.sampleRate);
    }

    super(
      globalThis.createAudioContext(
        options?.sampleRate || AudioManager.getDevicePreferredSampleRate()
      )
    );

    this.errorSubscription = this.audioEventEmitter.addAudioEventListener(
      'contextError',
      () => this.onerror?.()
    );
    (this.context as IAudioContext).onerror =
      this.errorSubscription.subscriptionId;
  }

  public get baseLatency(): number {
    return (this.context as IAudioContext).baseLatency;
  }

  public get outputLatency(): number {
    return (this.context as IAudioContext).outputLatency;
  }

  async close(): Promise<undefined> {
    if (this._state === 'closed') {
      throw new InvalidStateError('Cannot close a closed audio context.');
    }

    return this.transitionTo('closed', () =>
      (this.context as IAudioContext).close()
    );
  }

  async resume(): Promise<undefined> {
    if (this._state === 'closed') {
      throw new InvalidStateError('Cannot resume a closed audio context.');
    }

    return this.transitionTo('running', () =>
      (this.context as IAudioContext).resume()
    );
  }

  async suspend(): Promise<undefined> {
    if (this._state === 'closed') {
      throw new InvalidStateError('Cannot suspend a closed audio context.');
    }

    return this.transitionTo('suspended', () =>
      (this.context as IAudioContext).suspend()
    );
  }

  /**
   * @internal Called by AudioScheduledSourceNode.start(). The native driver
   * can start implicitly from the first scheduled source, with no promise to
   * carry the transition, so this records the control-thread state and issues
   * the resume whose resolution publishes it.
   */
  public override markRunningOnSourceStart(): void {
    if (this._state === 'suspended') {
      this.transitionTo('running', () =>
        (this.context as IAudioContext).resume()
      ).catch(() => {
        // Nothing awaits this transition (start() carries no promise for it);
        // transitionTo() already restored the control-thread state.
      });
    }
  }

  private async transitionTo(
    nextState: ContextState,
    nativeTransition: () => Promise<undefined>
  ): Promise<undefined> {
    this.setControlState(nextState);

    try {
      return await nativeTransition();
    } catch (error) {
      this.setControlState(this.state);
      throw error;
    }
  }

  createMediaElementSource(
    mediaElement: AudioTagHandle
  ): MediaElementAudioSourceNode {
    return new MediaElementAudioSourceNode(this, { mediaElement });
  }
}
