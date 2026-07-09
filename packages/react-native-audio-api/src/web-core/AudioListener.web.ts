import AudioParam from './AudioParam.web';
import BaseAudioContext from './BaseAudioContext.web';

export default class AudioListener {
  readonly context: BaseAudioContext;
  readonly listener: globalThis.AudioListener;

  readonly positionX: AudioParam;
  readonly positionY: AudioParam;
  readonly positionZ: AudioParam;
  readonly forwardX: AudioParam;
  readonly forwardY: AudioParam;
  readonly forwardZ: AudioParam;
  readonly upX: AudioParam;
  readonly upY: AudioParam;
  readonly upZ: AudioParam;

  constructor(context: BaseAudioContext, listener: globalThis.AudioListener) {
    this.context = context;
    this.listener = listener;

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
