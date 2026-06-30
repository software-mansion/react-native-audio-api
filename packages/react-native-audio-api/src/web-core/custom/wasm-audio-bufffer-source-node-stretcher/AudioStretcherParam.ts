import AudioParam from '../../AudioParam.web';
import BaseAudioContext from '../../BaseAudioContext.web';

const RAMP_SAMPLE_INTERVAL = 0.05;

type SetValueEvent = {
  type: 'setValue';
  value: number;
  time: number;
};

type LinearRampEvent = {
  type: 'linearRamp';
  value: number;
  time: number;
  rampStartTime: number;
  startValue: number;
};

type ExponentialRampEvent = {
  type: 'exponentialRamp';
  value: number;
  time: number;
  rampStartTime: number;
  startValue: number;
};

type SetTargetEvent = {
  type: 'setTarget';
  target: number;
  startValue: number;
  time: number;
  timeConstant: number;
};

type ValueCurveEvent = {
  type: 'valueCurve';
  values: Float32Array;
  time: number;
  duration: number;
};

export type StretcherAutomationEvent =
  | SetValueEvent
  | LinearRampEvent
  | ExponentialRampEvent
  | SetTargetEvent
  | ValueCurveEvent;

type AutomationSample = {
  value: number;
  time: number;
};

export default class AudioStretcherParam extends AudioParam {
  override readonly defaultValue: number;
  override readonly minValue: number;
  override readonly maxValue: number;
  private readonly _initialValue: number;
  private _events: StretcherAutomationEvent[] = [];
  private readonly _onChange:
    | ((event: StretcherAutomationEvent) => void)
    | null;

  private readonly _onCancel: ((cancelTime: number) => void) | null;

  constructor(
    context: BaseAudioContext,
    initialValue: number,
    defaultValue: number,
    minValue: number,
    maxValue: number,
    onChange?: (event: StretcherAutomationEvent) => void,
    onCancel?: (cancelTime: number) => void
  ) {
    const source = new globalThis.ConstantSourceNode(context.context, {
      offset: initialValue,
    });
    source.start(0);
    super(source.offset, context);
    this.defaultValue = defaultValue;
    this.minValue = minValue;
    this.maxValue = maxValue;
    this._initialValue = initialValue;
    this._onChange = onChange ?? null;
    this._onCancel = onCancel ?? null;
  }

  override get value(): number {
    return super.value;
  }

  override set value(v: number) {
    super.value = v;
    this._events = [
      {
        type: 'setValue',
        value: v,
        time: this.context.currentTime,
      },
    ];
    this._onChange?.({
      type: 'setValue',
      value: v,
      time: this.context.currentTime,
    });
  }

  getValueAtTime(time: number): number {
    if (this._events.length === 0) {
      return this._initialValue;
    }

    let value = this._initialValue;
    let tPrev = -Infinity;
    const sorted = [...this._events].sort((a, b) => a.time - b.time);

    for (const event of sorted) {
      if (event.type === 'setValue') {
        if (time < event.time) {
          return value;
        }
        value = event.value;
        tPrev = event.time;
        continue;
      }

      if (event.type === 'linearRamp') {
        const rampStart = event.rampStartTime ?? tPrev;
        if (time < rampStart) {
          continue;
        }
        if (time >= event.time) {
          value = event.value;
          tPrev = event.time;
          continue;
        }
        const v0 = event.startValue ?? value;
        const fraction = (time - rampStart) / (event.time - rampStart);
        return v0 + fraction * (event.value - v0);
      }

      if (event.type === 'exponentialRamp') {
        const rampStart = event.rampStartTime ?? tPrev;
        if (time < rampStart) {
          continue;
        }
        if (time >= event.time) {
          value = event.value;
          tPrev = event.time;
          continue;
        }
        const v0 = event.startValue ?? value;
        if (v0 === 0 || event.value === 0) {
          return v0;
        }
        const fraction = (time - rampStart) / (event.time - rampStart);
        return v0 * Math.pow(event.value / v0, fraction);
      }

      if (event.type === 'setTarget') {
        if (time < event.time) {
          return value;
        }
        const dt = time - event.time;
        const v0 = event.startValue;
        return (
          event.target +
          (v0 - event.target) * Math.exp(-dt / event.timeConstant)
        );
      }

      if (event.type === 'valueCurve') {
        if (time < event.time) {
          return value;
        }
        if (time >= event.time + event.duration) {
          value = event.values[event.values.length - 1];
          tPrev = event.time + event.duration;
          continue;
        }
        const progress = (time - event.time) / event.duration;
        const idx = progress * (event.values.length - 1);
        const i0 = Math.floor(idx);
        const i1 = Math.min(i0 + 1, event.values.length - 1);
        const frac = idx - i0;
        return event.values[i0] + frac * (event.values[i1] - event.values[i0]);
      }
    }

    return value;
  }

  getRampStartTime(endTime: number): number {
    let tPrev = -Infinity;
    const sorted = [...this._events].sort((a, b) => a.time - b.time);

    for (const event of sorted) {
      if (event.time >= endTime) {
        break;
      }
      tPrev = event.time;
    }

    if (tPrev === -Infinity) {
      return this.context.currentTime;
    }

    return tPrev;
  }

  sampleAutomation(
    event: StretcherAutomationEvent,
    mapValue: (value: number) => number
  ): AutomationSample[] {
    const samples: AutomationSample[] = [];

    if (event.type === 'setValue') {
      samples.push({
        value: mapValue(event.value),
        time: event.time,
      });
      return samples;
    }

    if (event.type === 'linearRamp' || event.type === 'exponentialRamp') {
      const startTime =
        event.rampStartTime ?? this.getRampStartTime(event.time);
      const endTime = event.time;
      const duration = endTime - startTime;

      if (duration <= 0) {
        samples.push({
          value: mapValue(event.value),
          time: endTime,
        });
        return samples;
      }

      const stepCount = Math.max(2, Math.ceil(duration / RAMP_SAMPLE_INTERVAL));
      for (let i = 0; i <= stepCount; i++) {
        const t = startTime + (duration * i) / stepCount;
        samples.push({
          value: mapValue(this.getValueAtTime(t)),
          time: t,
        });
      }
      return samples;
    }

    if (event.type === 'setTarget') {
      const startTime = event.time;
      const duration = Math.max(event.timeConstant * 5, RAMP_SAMPLE_INTERVAL);
      const stepCount = Math.max(2, Math.ceil(duration / RAMP_SAMPLE_INTERVAL));
      for (let i = 0; i <= stepCount; i++) {
        const t = startTime + (duration * i) / stepCount;
        samples.push({
          value: mapValue(this.getValueAtTime(t)),
          time: t,
        });
      }
      return samples;
    }

    if (event.type === 'valueCurve') {
      const stepCount = Math.max(
        2,
        Math.ceil(event.duration / RAMP_SAMPLE_INTERVAL)
      );
      for (let i = 0; i <= stepCount; i++) {
        const t = event.time + (event.duration * i) / stepCount;
        samples.push({
          value: mapValue(this.getValueAtTime(t)),
          time: t,
        });
      }
      return samples;
    }

    return samples;
  }

  private _recordEvent(event: StretcherAutomationEvent): void {
    this._events.push(event);
  }

  private _removeEventsFrom(cancelTime: number): void {
    this._events = this._events.filter((event) => event.time < cancelTime);
  }

  override setValueAtTime(value: number, startTime: number): AudioParam {
    const event: SetValueEvent = {
      type: 'setValue',
      value,
      time: startTime,
    };
    this._recordEvent(event);
    this._onChange?.(event);
    return this;
  }

  override linearRampToValueAtTime(value: number, endTime: number): AudioParam {
    const rampStartTime = this.getRampStartTime(endTime);
    const event: LinearRampEvent = {
      type: 'linearRamp',
      value,
      time: endTime,
      rampStartTime,
      startValue: this.getValueAtTime(rampStartTime),
    };
    this._recordEvent(event);
    this._onChange?.(event);
    return this;
  }

  override exponentialRampToValueAtTime(
    value: number,
    endTime: number
  ): AudioParam {
    const rampStartTime = this.getRampStartTime(endTime);
    const event: ExponentialRampEvent = {
      type: 'exponentialRamp',
      value,
      time: endTime,
      rampStartTime,
      startValue: this.getValueAtTime(rampStartTime),
    };
    this._recordEvent(event);
    this._onChange?.(event);
    return this;
  }

  override setTargetAtTime(
    target: number,
    startTime: number,
    timeConstant: number
  ): AudioParam {
    const event: SetTargetEvent = {
      type: 'setTarget',
      target,
      startValue: this.getValueAtTime(startTime),
      time: startTime,
      timeConstant,
    };
    this._recordEvent(event);
    this._onChange?.(event);
    return this;
  }

  override setValueCurveAtTime(
    values: Float32Array,
    startTime: number,
    duration: number
  ): AudioParam {
    const event: ValueCurveEvent = {
      type: 'valueCurve',
      values,
      time: startTime,
      duration,
    };
    this._recordEvent(event);
    this._onChange?.(event);
    return this;
  }

  override cancelScheduledValues(cancelTime: number): AudioParam {
    this._removeEventsFrom(cancelTime);
    this._onCancel?.(cancelTime);
    return this;
  }

  override cancelAndHoldAtTime(cancelTime: number): AudioParam {
    this._removeEventsFrom(cancelTime);
    this._onCancel?.(cancelTime);
    return this;
  }
}
