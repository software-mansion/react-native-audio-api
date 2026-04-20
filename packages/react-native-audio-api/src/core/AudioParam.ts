import { AutomationEventData, AutomationEventType } from '../types';
import { InvalidStateError, NotSupportedError, RangeError } from '../errors';
import { IAudioParam } from '../interfaces';
import type BaseAudioContext from './BaseAudioContext';

export default class AudioParam {
  public readonly defaultValue: number;
  public readonly minValue: number;
  public readonly maxValue: number;

  constructor(
    public readonly audioParam: IAudioParam,
    public readonly context: BaseAudioContext
  ) {
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

  public setValueAtTime(value: number, startTime: number): AudioParam {
    if (startTime < 0) {
      throw new RangeError(
        `startTime must be a finite non-negative number: ${startTime}`
      );
    }

    this.checkCurveExclusion({
      type: AutomationEventType.SET_VALUE,
      automationTime: startTime,
    });

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

    this.checkCurveExclusion({
      type: AutomationEventType.LINEAR_RAMP,
      automationTime: endTime,
    });

    const clampedTime = Math.max(endTime, this.context.currentTime);
    this.audioParam.linearRampToValueAtTime(value, clampedTime);

    return this;
  }

  public exponentialRampToValueAtTime(
    value: number,
    endTime: number
  ): AudioParam {
    if (value === 0) {
      throw new RangeError(`value must be a non-zero number: ${value}`);
    }

    if (endTime <= 0) {
      throw new RangeError(
        `endTime must be a finite non-negative number: ${endTime}`
      );
    }

    this.checkCurveExclusion({
      type: AutomationEventType.EXPONENTIAL_RAMP,
      automationTime: endTime,
    });

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

    this.checkCurveExclusion({
      type: AutomationEventType.SET_TARGET,
      automationTime: startTime,
    });

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

    this.checkCurveExclusion({
      type: AutomationEventType.SET_VALUE_CURVE,
      automationTime: startTime,
      duration,
    });

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

  private checkCurveExclusion(eventData: AutomationEventData): void {
    const checkExclusionResult = this.audioParam.checkCurveExclusion(eventData);

    if (checkExclusionResult.status === 'error') {
      throw new NotSupportedError(checkExclusionResult.message);
    }
  }
}
