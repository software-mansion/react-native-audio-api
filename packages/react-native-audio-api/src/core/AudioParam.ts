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

    this.audioParam.setValueAtTime(value, startTime);

    return this;
  }

  public linearRampToValueAtTime(value: number, endTime: number): AudioParam {
    if (endTime < 0) {
      throw new RangeError(
        `endTime must be a finite non-negative number: ${endTime}`
      );
    }

    this.audioParam.linearRampToValueAtTime(value, endTime);

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

    this.audioParam.exponentialRampToValueAtTime(value, endTime);

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

    this.audioParam.setTargetAtTime(target, startTime, timeConstant);

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

    this.audioParam.setValueCurveAtTime(values, startTime, duration);

    return this;
  }

  public cancelScheduledValues(cancelTime: number): AudioParam {
    if (cancelTime < 0) {
      throw new RangeError(
        `cancelTime must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.audioParam.cancelScheduledValues(cancelTime);

    return this;
  }

  public cancelAndHoldAtTime(cancelTime: number): AudioParam {
    if (cancelTime < 0) {
      throw new RangeError(
        `cancelTime must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.audioParam.cancelAndHoldAtTime(cancelTime);

    return this;
  }
}
