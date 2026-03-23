import { AutomationEventType, Result } from '../../types';
import { AutomationEvent } from './AutomationEvent';
import { BoundedPriorityQueue } from './BoundedPriorityQueue';

/**
 * A priority queue of automation events, ordered by start time. Enforces the
 * curve exclusion rule from the Web Audio spec:
 * https://webaudio.github.io/web-audio-api/#AudioParam. The queue is bounded to
 * a fixed capacity, and rejects new events when full. This is a utility used by
 * AudioParam for scheduling automation events.
 *
 * @param capacity - Maximum number of events the queue can hold. Must be a
 *   power of two greater than zero.
 * @throws NotSupportedError if a new event violates the curve exclusion rule
 *   with existing events in the queue.
 */
export class AutomationEventQueue {
  private readonly queue: BoundedPriorityQueue<AutomationEvent>;

  constructor(capacity: number = 32) {
    this.queue = new BoundedPriorityQueue<AutomationEvent>(
      capacity,
      (a, b) => a.automationTime < b.automationTime
    );
  }

  /**
   * Push a new automation event into the queue, enforcing the curve exclusion
   * rule.
   *
   * @param event - The automation event to be scheduled.
   * @returns True if the event was successfully scheduled, false if the queue
   *   is full.
   * @throws NotSupportedError if the event violates the curve exclusion rule
   *   with existing events in the queue.
   */
  public push(event: AutomationEvent): Result<{ value: boolean }> {
    const curveExclusionResult = this.satisfiesCurveExclusion(event);
    if (curveExclusionResult.status === 'error') {
      return curveExclusionResult;
    }
    return { status: 'success', value: this.queue.push(event) };
  }

  // the curve exclusion rule (Web Audio spec §1.6).
  // https://webaudio.github.io/web-audio-api/#AudioParam
  private satisfiesCurveExclusion(newEvent: AutomationEvent): Result<{}> {
    const newT = newEvent.startTime;
    const isSetValueCurve =
      newEvent.type === AutomationEventType.SetValueCurveAtTime;
    const newD = isSetValueCurve ? newEvent.endTime - newEvent.startTime : 0;

    for (let i = 0; i < this.queue.size; i++) {
      const existing = this.queue.peekAt(i);
      const existingT = existing.startTime;

      // 1. Any event scheduled inside an existing curve's [T, T+D) is not allowed.
      if (existing.type === AutomationEventType.SetValueCurveAtTime) {
        const existingEndTime = existing.endTime;
        if (newT >= existingT && newT < existingEndTime) {
          return {
            status: 'error',
            message: `Cannot schedule event ${newEvent.type} at time ${newT} because it overlaps with an existing SetValueCurveAtTime event from ${existingT} to ${existingEndTime}`,
          };
        }
      }

      // 2. A new curve may not contain any existing event in its open interval (T, T+D).
      if (isSetValueCurve && existingT > newT && existingT < newT + newD) {
        return {
          status: 'error',
          message: `Cannot schedule SetValueCurveAtTime event from ${newT} to ${newT + newD} because it overlaps with an existing event ${existing.type} at time ${existingT}`,
        };
      }
    }

    return { status: 'success' };
  }
}
