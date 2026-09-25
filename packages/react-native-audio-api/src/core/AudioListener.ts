import { IAudioListener } from '../jsi-interfaces';
import AudioParam from './AudioParam';
import type BaseAudioContext from './BaseAudioContext';

/**
 * Represents the position and orientation of the person listening to the audio
 * scene. All PannerNode objects spatialize in relation to the
 * BaseAudioContext's listener.
 */
export default class AudioListener {
  readonly context: BaseAudioContext;

  readonly positionX: AudioParam;
  readonly positionY: AudioParam;
  readonly positionZ: AudioParam;
  readonly forwardX: AudioParam;
  readonly forwardY: AudioParam;
  readonly forwardZ: AudioParam;
  readonly upX: AudioParam;
  readonly upY: AudioParam;
  readonly upZ: AudioParam;

  constructor(context: BaseAudioContext, listener: IAudioListener) {
    this.context = context;

    this.positionX = new AudioParam(listener.positionX, context);
    this.positionY = new AudioParam(listener.positionY, context);
    this.positionZ = new AudioParam(listener.positionZ, context);
    this.forwardX = new AudioParam(listener.forwardX, context);
    this.forwardY = new AudioParam(listener.forwardY, context);
    this.forwardZ = new AudioParam(listener.forwardZ, context);
    this.upX = new AudioParam(listener.upX, context);
    this.upY = new AudioParam(listener.upY, context);
    this.upZ = new AudioParam(listener.upZ, context);
  }
}
