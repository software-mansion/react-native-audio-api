import AudioParam from '../../AudioParam.web';
import BaseAudioContext from '../../BaseAudioContext.web';

export default class AudioStretcherParam extends AudioParam {
  override readonly defaultValue: number;
  override readonly minValue: number;
  override readonly maxValue: number;
  private readonly _onChange: ((value: number, time?: number) => void) | null;

  constructor(
    context: BaseAudioContext,
    initialValue: number,
    defaultValue: number,
    minValue: number,
    maxValue: number,
    onChange?: (value: number, time?: number) => void
  ) {
    const source = new globalThis.ConstantSourceNode(context.context, {
      offset: initialValue,
    });
    super(source.offset, context);
    this.defaultValue = defaultValue;
    this.minValue = minValue;
    this.maxValue = maxValue;
    this._onChange = onChange ?? null;
  }

  override get value(): number {
    return super.value;
  }

  override set value(v: number) {
    super.value = v;
    this._onChange?.(v);
  }

  override setValueAtTime(value: number, startTime: number): AudioParam {
    super.setValueAtTime(value, startTime);
    this._onChange?.(value, startTime);
    return this;
  }
}
