import { RangeError } from '../errors';

export default class AudioParam {
  readonly defaultValue: number;
  readonly minValue: number;
  readonly maxValue: number;

  private readonly param: globalThis.AudioParam;

  constructor(param: globalThis.AudioParam) {
    this.param = param;
    this.defaultValue = param.defaultValue;
    this.minValue = param.minValue;
    this.maxValue = param.maxValue;
  }

  public get value(): number {
    return this.param.value;
  }

  public set value(value: number) {
    this.param.value = value;
  }

  public setValueAtTime(value: number, startTime: number): void {
    if (startTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${startTime}`
      );
    }

    this.param.setValueAtTime(value, startTime);
  }

  public linearRampToValueAtTime(value: number, endTime: number): void {
    if (endTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${endTime}`
      );
    }

    this.param.linearRampToValueAtTime(value, endTime);
  }

  public exponentialRampToValueAtTime(value: number, endTime: number): void {
    if (endTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${endTime}`
      );
    }

    this.param.exponentialRampToValueAtTime(value, endTime);
  }

  public setTargetAtTime(
    target: number,
    startTime: number,
    timeConstant: number
  ): void {
    if (startTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${startTime}`
      );
    }

    this.param.setTargetAtTime(target, startTime, timeConstant);
  }

  public setValueCurveAtTime(
    values: number[],
    startTime: number,
    duration: number
  ): void {
    if (startTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${startTime}`
      );
    }

    this.param.setValueCurveAtTime(
      new Float32Array(values),
      startTime,
      duration
    );
  }

  public cancelScheduledValues(cancelTime: number): void {
    if (cancelTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.param.cancelScheduledValues(cancelTime);
  }

  public cancelAndHoldAtTime(cancelTime: number): void {
    if (cancelTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.param.cancelAndHoldAtTime(cancelTime);
  }
}
