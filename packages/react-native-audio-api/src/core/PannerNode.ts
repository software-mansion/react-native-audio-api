import { IPannerNode } from '../jsi-interfaces';
import { DistanceModelType, PannerOptions, PanningModelType } from '../types';
import { InvalidStateError, RangeError } from '../errors';
import { PannerOptionsValidator } from '../utils/validation';
import AudioNode from './AudioNode';
import AudioParam from './AudioParam';
import type BaseAudioContext from './BaseAudioContext';

export default class PannerNode extends AudioNode {
  readonly positionX: AudioParam;
  readonly positionY: AudioParam;
  readonly positionZ: AudioParam;
  readonly orientationX: AudioParam;
  readonly orientationY: AudioParam;
  readonly orientationZ: AudioParam;

  constructor(context: BaseAudioContext, options?: PannerOptions) {
    PannerOptionsValidator.validate(options);
    const panner: IPannerNode = context.context.createPanner(options || {});
    super(context, panner);
    this.positionX = new AudioParam(panner.positionX, context, this);
    this.positionY = new AudioParam(panner.positionY, context, this);
    this.positionZ = new AudioParam(panner.positionZ, context, this);
    this.orientationX = new AudioParam(panner.orientationX, context, this);
    this.orientationY = new AudioParam(panner.orientationY, context, this);
    this.orientationZ = new AudioParam(panner.orientationZ, context, this);
  }

  public get panningModel(): PanningModelType {
    return (this.node as IPannerNode).panningModel;
  }

  public set panningModel(value: PanningModelType) {
    (this.node as IPannerNode).panningModel = value;
  }

  public get distanceModel(): DistanceModelType {
    return (this.node as IPannerNode).distanceModel;
  }

  public set distanceModel(value: DistanceModelType) {
    (this.node as IPannerNode).distanceModel = value;
  }

  public get refDistance(): number {
    return (this.node as IPannerNode).refDistance;
  }

  public set refDistance(value: number) {
    if (value < 0) {
      throw new RangeError('refDistance must be non-negative');
    }
    (this.node as IPannerNode).refDistance = value;
  }

  public get maxDistance(): number {
    return (this.node as IPannerNode).maxDistance;
  }

  public set maxDistance(value: number) {
    if (value <= 0) {
      throw new RangeError('maxDistance must be positive');
    }
    (this.node as IPannerNode).maxDistance = value;
  }

  public get rolloffFactor(): number {
    return (this.node as IPannerNode).rolloffFactor;
  }

  public set rolloffFactor(value: number) {
    if (value < 0) {
      throw new RangeError('rolloffFactor must be non-negative');
    }
    (this.node as IPannerNode).rolloffFactor = value;
  }

  public get coneInnerAngle(): number {
    return (this.node as IPannerNode).coneInnerAngle;
  }

  public set coneInnerAngle(value: number) {
    (this.node as IPannerNode).coneInnerAngle = value;
  }

  public get coneOuterAngle(): number {
    return (this.node as IPannerNode).coneOuterAngle;
  }

  public set coneOuterAngle(value: number) {
    (this.node as IPannerNode).coneOuterAngle = value;
  }

  public get coneOuterGain(): number {
    return (this.node as IPannerNode).coneOuterGain;
  }

  public set coneOuterGain(value: number) {
    if (value < 0 || value > 1) {
      throw new InvalidStateError('coneOuterGain must be between 0 and 1');
    }
    (this.node as IPannerNode).coneOuterGain = value;
  }

  public setPosition(x: number, y: number, z: number): void {
    this.positionX.value = x;
    this.positionY.value = y;
    this.positionZ.value = z;
  }

  public setOrientation(x: number, y: number, z: number): void {
    this.orientationX.value = x;
    this.orientationY.value = y;
    this.orientationZ.value = z;
  }
}
