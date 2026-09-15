import { IAudioListener } from '../jsi-interfaces';
import { assertFloat32Representable } from '../utils/float32';
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

  /** @deprecated Use the `positionX/Y/Z` AudioParams instead. */
  public setPosition(x: number, y: number, z: number): void {
    assertFloat32Representable(x);
    assertFloat32Representable(y);
    assertFloat32Representable(z);
    this.positionX.value = x;
    this.positionY.value = y;
    this.positionZ.value = z;
  }

  /** @deprecated Use the `forwardX/Y/Z` and `upX/Y/Z` AudioParams instead. */
  public setOrientation(
    forwardX: number,
    forwardY: number,
    forwardZ: number,
    upX: number,
    upY: number,
    upZ: number
  ): void {
    assertFloat32Representable(forwardX);
    assertFloat32Representable(forwardY);
    assertFloat32Representable(forwardZ);
    assertFloat32Representable(upX);
    assertFloat32Representable(upY);
    assertFloat32Representable(upZ);
    this.forwardX.value = forwardX;
    this.forwardY.value = forwardY;
    this.forwardZ.value = forwardZ;
    this.upX.value = upX;
    this.upY.value = upY;
    this.upZ.value = upZ;
  }
}
