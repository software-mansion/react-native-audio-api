import { DistanceModelType, PannerOptions, PanningModelType } from '../types';
import AudioNode from './AudioNode.web';
import AudioParam from './AudioParam.web';
import BaseAudioContext from './BaseAudioContext.web';

export default class PannerNode extends AudioNode {
  readonly positionX: AudioParam;
  readonly positionY: AudioParam;
  readonly positionZ: AudioParam;
  readonly orientationX: AudioParam;
  readonly orientationY: AudioParam;
  readonly orientationZ: AudioParam;

  constructor(context: BaseAudioContext, pannerOptions?: PannerOptions) {
    const panner = new globalThis.PannerNode(context.context, pannerOptions);
    super(context, panner);
    this.positionX = new AudioParam(panner.positionX, context);
    this.positionY = new AudioParam(panner.positionY, context);
    this.positionZ = new AudioParam(panner.positionZ, context);
    this.orientationX = new AudioParam(panner.orientationX, context);
    this.orientationY = new AudioParam(panner.orientationY, context);
    this.orientationZ = new AudioParam(panner.orientationZ, context);
  }

  get panningModel(): PanningModelType {
    return (this.node as globalThis.PannerNode)
      .panningModel as PanningModelType;
  }

  set panningModel(value: PanningModelType) {
    (this.node as globalThis.PannerNode).panningModel = value;
  }

  get distanceModel(): DistanceModelType {
    return (this.node as globalThis.PannerNode)
      .distanceModel as DistanceModelType;
  }

  set distanceModel(value: DistanceModelType) {
    (this.node as globalThis.PannerNode).distanceModel = value;
  }

  get refDistance(): number {
    return (this.node as globalThis.PannerNode).refDistance;
  }

  set refDistance(value: number) {
    (this.node as globalThis.PannerNode).refDistance = value;
  }

  get maxDistance(): number {
    return (this.node as globalThis.PannerNode).maxDistance;
  }

  set maxDistance(value: number) {
    (this.node as globalThis.PannerNode).maxDistance = value;
  }

  get rolloffFactor(): number {
    return (this.node as globalThis.PannerNode).rolloffFactor;
  }

  set rolloffFactor(value: number) {
    (this.node as globalThis.PannerNode).rolloffFactor = value;
  }

  get coneInnerAngle(): number {
    return (this.node as globalThis.PannerNode).coneInnerAngle;
  }

  set coneInnerAngle(value: number) {
    (this.node as globalThis.PannerNode).coneInnerAngle = value;
  }

  get coneOuterAngle(): number {
    return (this.node as globalThis.PannerNode).coneOuterAngle;
  }

  set coneOuterAngle(value: number) {
    (this.node as globalThis.PannerNode).coneOuterAngle = value;
  }

  get coneOuterGain(): number {
    return (this.node as globalThis.PannerNode).coneOuterGain;
  }

  set coneOuterGain(value: number) {
    (this.node as globalThis.PannerNode).coneOuterGain = value;
  }

  setPosition(x: number, y: number, z: number): void {
    (this.node as globalThis.PannerNode).setPosition(x, y, z);
  }

  setOrientation(x: number, y: number, z: number): void {
    (this.node as globalThis.PannerNode).setOrientation(x, y, z);
  }
}
