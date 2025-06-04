import { RangeError, InvalidStateError } from '../errors';
import BaseAudioContext from './BaseAudioContext';

export default class AudioParam {
  readonly defaultValue: number;
  readonly minValue: number;
  readonly maxValue: number;
  readonly context: BaseAudioContext;

  readonly param: globalThis.AudioParam;

  constructor(param: globalThis.AudioParam, context: BaseAudioContext) {
    this.param = param;
    this.defaultValue = param.defaultValue;
    this.minValue = param.minValue;
    this.maxValue = param.maxValue;
    this.context = context;
  }

  public get value(): number {
    return this.param.value;
  }

  public set value(value: number) {
    this.param.value = value;
  }

  public setValueAtTime(value: number, startTime: number): AudioParam {
    if (startTime < 0) {
      throw new RangeError(
        `startTime must be a finite non-negative number: ${startTime}`
      );
    }

    this.param.setValueAtTime(value, startTime);

    return this;
  }

  public linearRampToValueAtTime(value: number, endTime: number): AudioParam {
    if (endTime < 0) {
      throw new RangeError(
        `endTime must be a finite non-negative number: ${endTime}`
      );
    }

    this.param.linearRampToValueAtTime(value, endTime);

    return this;
  }

  public exponentialRampToValueAtTime(
    value: number,
    endTime: number
  ): AudioParam {
    if (endTime < 0) {
      throw new RangeError(
        `endTime must be a finite non-negative number: ${endTime}`
      );
    }

    this.param.exponentialRampToValueAtTime(value, endTime);

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
        `timeConstant must be a finite non-negative number: ${startTime}`
      );
    }

    this.param.setTargetAtTime(target, startTime, timeConstant);

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

    if (duration < 0) {
      throw new RangeError(
        `duration must be a finite non-negative number: ${startTime}`
      );
    }

    if (values.length < 2) {
      throw new InvalidStateError(`values must contain at least two values`);
    }

    this.param.setValueCurveAtTime(values, startTime, duration);

    return this;
  }

  public cancelScheduledValues(cancelTime: number): AudioParam {
    if (cancelTime < 0) {
      throw new RangeError(
        `cancelTime must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.param.cancelScheduledValues(cancelTime);

    return this;
  }

  public cancelAndHoldAtTime(cancelTime: number): AudioParam {
    if (cancelTime < 0) {
      throw new RangeError(
        `cancelTime must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.param.cancelAndHoldAtTime(cancelTime);

    return this;
  }
}
