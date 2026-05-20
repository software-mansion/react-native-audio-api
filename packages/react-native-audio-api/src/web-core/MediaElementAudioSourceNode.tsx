import AudioNode from './AudioNode';
import type BaseAudioContext from './BaseAudioContext';

export interface MediaElementAudioSourceOptions {
  mediaElement: HTMLMediaElement;
}

export default class MediaElementAudioSourceNode extends AudioNode {
  constructor(
    context: BaseAudioContext,
    node: globalThis.MediaElementAudioSourceNode,
    readonly mediaElement: HTMLMediaElement
  ) {
    super(context, node);
  }
}
