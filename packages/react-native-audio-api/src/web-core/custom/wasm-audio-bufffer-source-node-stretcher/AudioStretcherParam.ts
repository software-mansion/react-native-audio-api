import AudioParam from '../../AudioParam.web';
import BaseAudioContext from '../../BaseAudioContext.web';

export default class AudioStretcherParam extends AudioParam {
  override readonly defaultValue: number;
  override readonly minValue: number;
  override readonly maxValue: number;

  constructor(
    context: BaseAudioContext,
    initialValue: number,
    defaultValue: number,
    minValue: number,
    maxValue: number
  ) {
    const source = new globalThis.ConstantSourceNode(context.context, {
      offset: initialValue,
    });
    super(source.offset, context);
    this.defaultValue = defaultValue;
    this.minValue = minValue;
    this.maxValue = maxValue;
  }
}
