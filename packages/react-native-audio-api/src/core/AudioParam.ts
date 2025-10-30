import { IAudioParam } from '../interfaces';
import { RangeError, InvalidStateError } from '../errors';
import BaseAudioContext from './BaseAudioContext';

export default class AudioParam {
  readonly defaultValue: number;
  readonly minValue: number;
  readonly maxValue: number;
  readonly audioParam: IAudioParam;
  readonly context: BaseAudioContext;

  constructor(audioParam: IAudioParam, context: BaseAudioContext) {
    this.audioParam = audioParam;
    this.value = audioParam.value;
    this.defaultValue = audioParam.defaultValue;
    this.minValue = audioParam.minValue;
    this.maxValue = audioParam.maxValue;
    this.context = context;
  }

  public get value(): number {
    return this.audioParam.value;
  }

  public set value(value: number) {
    this.audioParam.value = value;
  }

  public setValueAtTime(value: number, startTime: number): AudioParam {
    if (startTime < 0) {
      throw new RangeError(
        `startTime must be a finite non-negative number: ${startTime}`
      );
    }

    const clampedTime = Math.max(startTime, this.context.currentTime);
    this.audioParam.setValueAtTime(value, clampedTime);

    return this;
  }

  public linearRampToValueAtTime(value: number, endTime: number): AudioParam {
    if (endTime < 0) {
      throw new RangeError(
        `endTime must be a finite non-negative number: ${endTime}`
      );
    }

    const clampedTime = Math.max(endTime, this.context.currentTime);
    this.audioParam.linearRampToValueAtTime(value, clampedTime);

    return this;
  }

  public exponentialRampToValueAtTime(
    value: number,
    endTime: number
  ): AudioParam {
    if (endTime <= 0) {
      throw new RangeError(
        `endTime must be a finite non-negative number: ${endTime}`
      );
    }

    const clampedTime = Math.max(endTime, this.context.currentTime);
    this.audioParam.exponentialRampToValueAtTime(value, clampedTime);

    return this;
  }

  public setTargetAtTime(
    target: number,
    startTime: number,
    timeConstant: number
  ): AudioParam {
    if (startTime < 0) {
      throw new RangeError(
        `startTime must be a finite non-negative number: ${startTime}`
      );
    }

    if (timeConstant < 0) {
      throw new RangeError(
        `timeConstant must be a finite non-negative number: ${timeConstant}`
      );
    }

    const clampedTime = Math.max(startTime, this.context.currentTime);
    this.audioParam.setTargetAtTime(target, clampedTime, timeConstant);

    return this;
  }

  public setValueCurveAtTime(
    values: Float32Array,
    startTime: number,
    duration: number
  ): AudioParam {
    if (startTime < 0) {
      throw new RangeError(
        `startTime must be a finite non-negative number: ${startTime}`
      );
    }

    if (duration <= 0) {
      throw new RangeError(
        `duration must be a finite strictly-positive number: ${duration}`
      );
    }

    if (values.length < 2) {
      throw new InvalidStateError(`values must contain at least two values`);
    }

    const clampedTime = Math.max(startTime, this.context.currentTime);
    this.audioParam.setValueCurveAtTime(values, clampedTime, duration);

    return this;
  }

  public cancelScheduledValues(cancelTime: number): AudioParam {
    if (cancelTime < 0) {
      throw new RangeError(
        `cancelTime must be a finite non-negative number: ${cancelTime}`
      );
    }

    const clampedTime = Math.max(cancelTime, this.context.currentTime);
    this.audioParam.cancelScheduledValues(clampedTime);

    return this;
  }

  public cancelAndHoldAtTime(cancelTime: number): AudioParam {
    if (cancelTime < 0) {
      throw new RangeError(
        `cancelTime must be a finite non-negative number: ${cancelTime}`
      );
    }

    const clampedTime = Math.max(cancelTime, this.context.currentTime);
    this.audioParam.cancelAndHoldAtTime(clampedTime);

    return this;
  }
}
