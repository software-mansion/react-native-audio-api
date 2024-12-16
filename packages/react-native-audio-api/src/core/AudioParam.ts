import { IAudioParam } from '../interfaces';
import { RangeError } from '../errors';

export default class AudioParam {
  readonly defaultValue: number;
  readonly minValue: number;
  readonly maxValue: number;
  private readonly audioParam: IAudioParam;

  constructor(audioParam: IAudioParam) {
    this.audioParam = audioParam;
    this.value = audioParam.value;
    this.defaultValue = audioParam.defaultValue;
    this.minValue = audioParam.minValue;
    this.maxValue = audioParam.maxValue;
  }

  public get value(): number {
    return this.audioParam.value;
  }

  public set value(value: number) {
    this.audioParam.value = value;
  }

  public setValueAtTime(value: number, startTime: number): void {
    if (startTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${startTime}`
      );
    }

    this.audioParam.setValueAtTime(value, startTime);
  }

  public linearRampToValueAtTime(value: number, endTime: number): void {
    if (endTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${endTime}`
      );
    }

    this.audioParam.linearRampToValueAtTime(value, endTime);
  }

  public exponentialRampToValueAtTime(value: number, endTime: number): void {
    if (endTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${endTime}`
      );
    }

    this.audioParam.exponentialRampToValueAtTime(value, endTime);
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

    this.audioParam.setTargetAtTime(target, startTime, timeConstant);
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

    this.audioParam.setValueCurveAtTime(values, startTime, duration);
  }

  public cancelScheduledValues(cancelTime: number): void {
    if (cancelTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.audioParam.cancelScheduledValues(cancelTime);
  }

  public cancelAndHoldAtTime(cancelTime: number): void {
    if (cancelTime < 0) {
      throw new RangeError(
        `Time must be a finite non-negative number: ${cancelTime}`
      );
    }

    this.audioParam.cancelAndHoldAtTime(cancelTime);
  }
}
