import { AutomationEventType } from '../../types';

export class AutomationEvent {
  // eslint-disable-next-line no-useless-constructor
  constructor(
    public readonly type: AutomationEventType,
    public readonly startTime: number,
    public readonly endTime: number
  ) {}

  public get automationTime(): number {
    const isRamp =
      this.type === AutomationEventType.LinearRampToValueAtTime ||
      this.type === AutomationEventType.ExponentialRampToValueAtTime;
    return isRamp ? this.endTime : this.startTime;
  }
}
